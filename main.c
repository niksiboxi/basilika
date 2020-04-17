/** @file
 * This file contains the source code for a sample application using ADC.
 */

#include "app_error.h"
#include "app_util_platform.h"
#include "boards.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_timer.h"
#include "nrf_pwr_mgmt.h"
#include "sdk_macros.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nrf_gpio.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define SAMPLES_IN_BUFFER 1

#define LED_RED 4
#define LED_GRN 28
#define LED_BLU 29

//#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)((((ADC_VALUE)*600)/256)*6)

static const nrf_drv_timer_t m_timer = NRF_DRV_TIMER_INSTANCE(0);
static nrf_saadc_value_t m_buffer[SAMPLES_IN_BUFFER];
nrf_saadc_value_t adc_result;
static uint8_t voltage_lvl_in_mill_volts;

static const nrfx_saadc_config_t saadc_config =
    {
        NRF_SAADC_RESOLUTION_8BIT,
        NRF_SAADC_OVERSAMPLE_DISABLED,
        0,
        false};

void saadc_callback(nrf_drv_saadc_evt_t const *p_event) {
  if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {
    ret_code_t err_code;

    err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);

    adc_result = p_event->data.done.p_buffer[0];

    voltage_lvl_in_mill_volts = adc_result * 600 / 256 * 6;

    NRF_LOG_INFO("ADC Reading in millivolts: %d", voltage_lvl_in_mill_volts);

    NRF_LOG_INFO("%d", p_event->data.done.p_buffer[0]);
  }
}

void saadc_init(void) {
  ret_code_t err_code;
  nrf_saadc_channel_config_t channel_config =
      NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1); // pin P0.03

  err_code = nrf_drv_saadc_init(&saadc_config, saadc_callback);
  APP_ERROR_CHECK(err_code);

  err_code = nrf_drv_saadc_channel_init(0, &channel_config);
  APP_ERROR_CHECK(err_code);

  err_code = nrf_drv_saadc_buffer_convert(m_buffer, SAMPLES_IN_BUFFER);
  APP_ERROR_CHECK(err_code);
}

int main(void) {
  uint32_t err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);

  NRF_LOG_DEFAULT_BACKENDS_INIT();

  saadc_init();
  NRF_LOG_INFO("SAADC HAL simple example started.");

  nrf_gpio_cfg_output(LED_RED);
  nrf_gpio_cfg_output(LED_GRN);
  nrf_gpio_cfg_output(LED_BLU);

  while (1) {
    //nrf_drv_saadc_sample();

    for (int i = 0; i < 3; i++) {
      nrf_delay_ms(1000);
      NRF_LOG_INFO("%d", i);
      if (i == 0) {
        nrf_gpio_pin_set(LED_RED);
        nrf_gpio_pin_clear(LED_GRN);
        nrf_gpio_pin_clear(LED_BLU);
      }
      if (i == 1) {
        nrf_gpio_pin_set(LED_GRN);
        nrf_gpio_pin_clear(LED_RED);
        nrf_gpio_pin_clear(LED_BLU);
      }
      if (i == 2) {
        nrf_gpio_pin_clear(LED_GRN);
        nrf_gpio_pin_clear(LED_GRN);
        nrf_gpio_pin_set(LED_BLU);
      }
    }
  }
}