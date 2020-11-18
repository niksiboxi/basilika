#include "app_error.h"
#include "boards.h"
#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "rtc.h"
#include "saadc.h"
#include "sdk_config.h"
#include <string.h>

#define NPN_TR_BASE 30

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

static void lfclk_config(void) {
  APP_ERROR_CHECK(nrf_drv_clock_init());

  nrf_drv_clock_lfclk_request(NULL);
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