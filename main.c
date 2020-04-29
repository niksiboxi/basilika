/** @file
 * This file contains the source code for a sample application using ADC.
 */

#include "app_error.h"
#include "app_util_platform.h"
#include "boards.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "nrf_delay.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_rtc.h"
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

#define UART_PRINTING_ENABLED

#define LED_RED 4
#define LED_GRN 28
#define LED_BLU 29

//#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)((((ADC_VALUE)*600)/256)*6)

//static const nrf_drv_timer_t m_timer = NRF_DRV_TIMER_INSTANCE(0);
static nrf_saadc_value_t m_buffer[SAMPLES_IN_BUFFER];
nrf_saadc_value_t adc_result;
static int16_t voltage_lvl_in_mill_volts;
const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */

// CHECK nRF5_SDK_16.0.0_98a08e2/examples//peripheral/rtc/main.c

// For money tree
void rgb_led_ctrl(int sample) {  
  if (sample > 102) {    // Oversaturation > 40%
    nrf_gpio_pin_clear(LED_RED);
    nrf_gpio_pin_clear(LED_GRN);
    nrf_gpio_pin_set(LED_BLU);
  } else if (sample >= 54  && sample <= 102) {   // Available Water 21%...40%
    nrf_gpio_pin_clear(LED_RED);
    nrf_gpio_pin_set(LED_GRN);
    nrf_gpio_pin_clear(LED_BLU);
  } else {    // Unavailable Water < 21%
    nrf_gpio_pin_set(LED_RED);
    nrf_gpio_pin_clear(LED_GRN);
    nrf_gpio_pin_clear(LED_BLU);
  }
}

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

    // Vin = ADCresult * Reference / (Resolution * Gain)
    // VDD = 2.84V
    voltage_lvl_in_mill_volts = (adc_result * 2840) / 255 * 1;

    #ifdef UART_PRINTING_ENABLED
    NRF_LOG_INFO("ADC: %d mV: %u", adc_result, voltage_lvl_in_mill_volts);
    #endif

    rgb_led_ctrl(p_event->data.done.p_buffer[0]);
  }
}

void saadc_init(void) {
  ret_code_t err_code;
  nrf_saadc_channel_config_t channel_config =
      NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1); // pin P0.03

  channel_config.reference = NRF_SAADC_REFERENCE_VDD4;  // VDD/4 as reference.
  channel_config.gain = SAADC_CH_CONFIG_GAIN_Gain1;     // 1x Gain

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
  #ifdef UART_PRINTING_ENABLED
  NRF_LOG_INFO("SAADC HAL simple example started.");
  #endif

  nrf_gpio_cfg_output(LED_RED);
  nrf_gpio_cfg_output(LED_GRN);
  nrf_gpio_cfg_output(LED_BLU);
  int x;

  while (1) {
    nrf_drv_saadc_sample();
    err_code = sd_app_evt_wait();
    //nrf_delay_ms(1000);
  }
}