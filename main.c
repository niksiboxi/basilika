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
#include "saadc.h"
#include <string.h>


#define NPN_TR_BASE 30
#define COMPARE_COUNTERTIME (10UL) /**< Get Compare event COMPARE_TIME seconds after the counter starts from 0. */

static volatile bool m_sampling = false;

void saadc_init_sample_uninit(void) {
  nrf_gpio_pin_set(NPN_TR_BASE);
  saadc_init();
  saadc_sample();
  while (m_sampling == true)
    ;
  saadc_uninit();
  nrf_gpio_pin_clear(NPN_TR_BASE);
}


const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */

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

int main(void) {
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

  //NRF_LOG_DEFAULT_BACKENDS_INIT();

  nrf_gpio_cfg_output(NPN_TR_BASE);

  lfclk_config();
  rtc_config();
  saadc_init_sample_uninit();

  while (1) {
    // Make sure any pending events are cleared
    __SEV();
    __WFE();
    // Enter System ON sleep mode
    __WFE();
  }
}