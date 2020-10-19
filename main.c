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

int main(void) {
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

  NRF_LOG_DEFAULT_BACKENDS_INIT();

  nrf_gpio_cfg_output(NPN_TR_BASE);
  gpio_init();
  lfclk_config();
  rtc_config();
  saadc_init_sample_uninit();
  spi_config();

  while (1) {
    // Make sure any pending events are cleared
    __SEV();
    __WFE();
    // Enter System ON sleep mode
    __WFE();
  }
}