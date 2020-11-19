/* RTC Clock Library Based on Nordic SDK
 *
 */

#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"
#include "saadc.h"

#define COMPARE_COUNTERTIME (1UL) /**< Get Compare event COMPARE_TIME seconds after the counter starts from 0. */

const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */

/* Function to handling the RTC0 interrupts.
 * Triggered on TICK and COMPARE0 match
 */
static void rtc_handler(nrf_drv_rtc_int_type_t int_type) {
  if (int_type == NRF_DRV_RTC_INT_COMPARE0) {
    saadc_init_sample_uninit();
    nrf_drv_rtc_counter_clear(&rtc);
  }
}

void rtc_config(void) {
  nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG; // Enable RTC instance
  config.prescaler = 4095;
  APP_ERROR_CHECK(nrf_drv_rtc_init(&rtc, &config, rtc_handler)); // Initialize the RTC driver instance

  nrf_drv_rtc_tick_enable(&rtc, true); // Enable tick event & interrupt

  APP_ERROR_CHECK(nrf_drv_rtc_cc_set(&rtc, 0, COMPARE_COUNTERTIME * 8, true)); // Set compare channel to trigger interrupt after COMPARE_COUNTERTIME seconds

  nrf_drv_rtc_enable(&rtc); // Power on RTC instance
}