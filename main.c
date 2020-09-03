#include "app_error.h"
#include "boards.h"
#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"
#include "nrf_drv_saadc.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "sdk_config.h"
#include <string.h>

#define SAMPLES_IN_BUFFER 1
#define NPN_TR_BASE 30
#define COMPARE_COUNTERTIME (10UL) /**< Get Compare event COMPARE_TIME seconds after the counter starts from 0. */

static nrf_saadc_value_t m_buffer[SAMPLES_IN_BUFFER];
static nrf_saadc_value_t adc_result;
static bool m_saadc_initialized = false;
static volatile bool m_sampling = false;

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
  saadc_init();
  saadc_sample();
  while (m_sampling == true)
    ;
  saadc_uninit();
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
    NRF_LOG_INFO("HELLO WORLD!");
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

int main(void) {
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

  NRF_LOG_DEFAULT_BACKENDS_INIT();

  nrf_gpio_cfg_output(NPN_TR_BASE);
  nrf_gpio_pin_set(NPN_TR_BASE);

  //saadc_init();
  lfclk_config();
  rtc_config();

  NRF_LOG_INFO("Init: done...");

  saadc_init_sample_uninit();

  while (1) {
    // Make sure any pending events are cleared
    __SEV();
    __WFE();
    // Enter System ON sleep mode
    __WFE();
  }
}