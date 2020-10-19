#include "app_error.h"
#include "boards.h"
#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "sdk_config.h"
#include <inttypes.h>
#include <string.h>

#define SAMPLES_IN_BUFFER 1
#define SPI_INSTANCE 0
#define NPN_TR_BASE 30
#define COMPARE_COUNTERTIME (10UL) /**< Get Compare event COMPARE_TIME seconds after the counter starts from 0. */

#define SPI_SCK_PIN NRF_GPIO_PIN_MAP(0, 27)
#define SPI_MISO_PIN NRF_GPIO_PIN_MAP(0, 26)
#define SPI_MOSI_PIN NRF_GPIO_PIN_MAP(0, 2)
#define SPI_SS_PIN NRF_GPIO_PIN_MAP(0, 25)
#define SPI_DC_PIN NRF_GPIO_PIN_MAP(0, 24)
#define SPI_RST_PIN NRF_GPIO_PIN_MAP(0, 23)
#define SPI_BUSY_PIN NRF_GPIO_PIN_MAP(0, 22)
#define NRF_SPIS_PIN_NOT_CONNECTED 0xFF

#define TEST_STRING "Nordic"

static nrf_saadc_value_t m_buffer[SAMPLES_IN_BUFFER];
static nrf_saadc_value_t adc_result;
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);

static bool m_saadc_initialized = false;
static volatile bool m_sampling = false;
static volatile bool spi_xfer_done;

static uint8_t m_tx_buf[] = TEST_STRING;
static const uint8_t m_length = sizeof(m_tx_buf);

const uint8_t lut_full_update[] = {
    0x80,
    0x60,
    0x40,
    0x00,
    0x00,
    0x00,
    0x00, //LUT0: BB:     VS 0 ~7
    0x10,
    0x60,
    0x20,
    0x00,
    0x00,
    0x00,
    0x00, //LUT1: BW:     VS 0 ~7
    0x80,
    0x60,
    0x40,
    0x00,
    0x00,
    0x00,
    0x00, //LUT2: WB:     VS 0 ~7
    0x10,
    0x60,
    0x20,
    0x00,
    0x00,
    0x00,
    0x00, //LUT3: WW:     VS 0 ~7
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, //LUT4: VCOM:   VS 0 ~7

    0x03,
    0x03,
    0x00,
    0x00,
    0x02, // TP0 A~D RP0
    0x09,
    0x09,
    0x00,
    0x00,
    0x02, // TP1 A~D RP1
    0x03,
    0x03,
    0x00,
    0x00,
    0x02, // TP2 A~D RP2
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // TP3 A~D RP3
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // TP4 A~D RP4
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // TP5 A~D RP5
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // TP6 A~D RP6

    0x15,
    0x41,
    0xA8,
    0x32,
    0x30,
    0x0A,
};

const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */

/* SAADC Driver Configuration */
static const nrf_drv_saadc_config_t saadc_config =
    {
        NRF_SAADC_RESOLUTION_8BIT,
        NRF_SAADC_OVERSAMPLE_DISABLED,
        0,
        true};

/* SAADC Callback */
void saadc_callback(nrf_drv_saadc_evt_t const *p_event) {
  if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {
    APP_ERROR_CHECK(nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER)); // start conversion in non-blocking mode

    adc_result = p_event->data.done.p_buffer[0];

    NRF_LOG_INFO("ADC: %d", adc_result);

    m_sampling = false;
  }
}

static void saadc_init(void) {
  nrf_saadc_channel_config_t channel_config =
      NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1); // Pin P0.03

  channel_config.reference = NRF_SAADC_REFERENCE_VDD4;
  channel_config.gain = SAADC_CH_CONFIG_GAIN_Gain1_6;
  channel_config.acq_time = NRF_SAADC_ACQTIME_3US;

  APP_ERROR_CHECK(nrf_drv_saadc_init(&saadc_config, saadc_callback));

  APP_ERROR_CHECK(nrf_drv_saadc_channel_init(0, &channel_config));

  APP_ERROR_CHECK(nrf_drv_saadc_buffer_convert(m_buffer, SAMPLES_IN_BUFFER));
}

static void saadc_uninit(void) {
  nrf_drv_saadc_uninit();
}

static void saadc_sample(void) {
  m_sampling = true;

  APP_ERROR_CHECK(nrf_drv_saadc_sample());
}

static void saadc_init_sample_uninit(void) {
  nrf_gpio_pin_set(NPN_TR_BASE);
  saadc_init();
  saadc_sample();
  while (m_sampling == true)
    ;
  saadc_uninit();
  nrf_gpio_pin_clear(NPN_TR_BASE);
}

static void lfclk_config(void) {
  APP_ERROR_CHECK(nrf_drv_clock_init());

  nrf_drv_clock_lfclk_request(NULL);
}

/* Function to handling the RTC0 interrupts.
 * Triggered on TICK and COMPARE0 match
 */
static void rtc_handler(nrf_drv_rtc_int_type_t int_type) {
  if (int_type == NRF_DRV_RTC_INT_COMPARE0) {
    saadc_init_sample_uninit();
    nrf_drv_rtc_counter_clear(&rtc);
  }
}

static void rtc_config(void) {
  nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG; // Enable RTC instance
  config.prescaler = 4095;
  APP_ERROR_CHECK(nrf_drv_rtc_init(&rtc, &config, rtc_handler)); // Initialize the RTC driver instance

  nrf_drv_rtc_tick_enable(&rtc, true); // Enable tick event & interrupt

  APP_ERROR_CHECK(nrf_drv_rtc_cc_set(&rtc, 0, COMPARE_COUNTERTIME * 8, true)); // Set compare channel to trigger interrupt after COMPARE_COUNTERTIME seconds

  nrf_drv_rtc_enable(&rtc); // Power on RTC instance
}

void spi_event_handler(nrf_drv_spi_evt_t const *p_event,
    void *p_context) {
}

void gpio_init(void) {
  nrf_gpio_cfg_output(SPI_SS_PIN);
  nrf_gpio_cfg_output(SPI_DC_PIN);
  nrf_gpio_cfg_output(SPI_RST_PIN);
  nrf_gpio_cfg_input(SPI_BUSY_PIN, GPIO_PIN_CNF_PULL_Disabled);  

  nrf_gpio_pin_set(SPI_RST_PIN);
}

void CSLow(void) {
  nrf_gpio_pin_clear(SPI_SS_PIN);
}

void CSHigh(void) {
  nrf_gpio_pin_set(SPI_SS_PIN);
}

void wait_until_idle(void) {
  while (nrf_gpio_pin_read(SPI_BUSY_PIN) != 0)  //LOW: idle, HIGH: busy
    NRF_LOG_INFO("BUSY: %d", nrf_gpio_pin_read(SPI_BUSY_PIN));
  ;
}

void spi_config(void) {
  nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;

  spi_config.sck_pin = SPI_SCK_PIN;
  spi_config.miso_pin = SPI_MISO_PIN;
  spi_config.mosi_pin = SPI_MOSI_PIN;
  spi_config.ss_pin = SPI_SS_PIN;

  spi_config.frequency = NRF_DRV_SPI_FREQ_2M;
  spi_config.bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
  spi_config.mode = NRF_DRV_SPI_MODE_0;

  APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
}

void send_command(uint8_t command) {
  uint8_t cmd[1] = {0};
  uint8_t rx_buf[] = {0};
  nrf_gpio_pin_clear(SPI_DC_PIN); // Low for command

  cmd[0] = command;

  while (nrf_drv_spi_transfer(&spi, cmd, 1, rx_buf, 1) == NRF_ERROR_BUSY)
    ;
}

void send_data(uint8_t data) {
  uint8_t dt[1] = {0};
  uint8_t rx_buf[] = {0};
  nrf_gpio_pin_set(SPI_DC_PIN); // High for data

  dt[0] = data;

  while (nrf_drv_spi_transfer(&spi, dt, 1, rx_buf, 1) == NRF_ERROR_BUSY)
    ;
}

void ePaper_init(void) {
  send_command(0x12); // Soft Reset
  wait_until_idle();

  send_command(0x74); // set analog block control
  send_data(0x54);
  send_command(0x7E); // set digital block contorl
  send_data(0x3B);

  send_command(0x01); // Driver output control
  send_data(0xF9);
  send_data(0x00);
  send_data(0x00);

  send_command(0x44); // data entry mode
  send_data(0x01);

  send_command(0x44); //set Ram-X address start/end position
  send_data(0x00);
  send_data(0x0F); //0x0C-->(15+1)*8=128

  send_command(0x45); //set Ram-Y address start/end position
  send_data(0xF9);    //0xF9-->(249+1)=250
  send_data(0x00);
  send_data(0x00);
  send_data(0x00);

  send_command(0x3C); //BorderWavefrom
  send_data(0x55);

  send_command(0x2C); // VCOM Voltage
  send_data(0x55);

  send_command(0x03);
  send_data(lut_full_update[70]);

  send_command(0x04);
  send_data(lut_full_update[71]);
  send_data(lut_full_update[72]);
  send_data(lut_full_update[73]);

  send_command(0x3A); // Dummy line
  send_data(lut_full_update[74]);
  send_command(0x3B); // Gate time
  send_data(lut_full_update[75]);

  send_command(0x32);
  for (int i = 0; i < 70; i++) {
    send_data(lut_full_update[i]);
  }

  send_command(0x4E); // set RAM x address count to 0
  send_data(0x00);
  send_command(0x4F); // set RAM y address count 0x127
  send_data(0xF9);
  send_data(0x00);
  wait_until_idle();
}

void ePaper_reset(void){
  nrf_gpio_pin_set(SPI_RST_PIN);
  nrf_delay_ms(200);
  nrf_gpio_pin_clear(SPI_RST_PIN);
  nrf_delay_ms(10);
  nrf_gpio_pin_set(SPI_RST_PIN);
  nrf_delay_ms(200);
}

int main(void) {
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

  NRF_LOG_DEFAULT_BACKENDS_INIT();

  nrf_gpio_cfg_output(NPN_TR_BASE);
  gpio_init();
  lfclk_config();
  rtc_config();
  saadc_init_sample_uninit();
  spi_config();

  //ePaper_init();
  ePaper_reset();

  while (1) {
    // Make sure any pending events are cleared
    __SEV();
    __WFE();
    // Enter System ON sleep mode
    __WFE();
  }
}