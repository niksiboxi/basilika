/*
 * SAADC interface implementation on Nordic SDK16 / nRF52
 *
 * Author: Nikita Pavlov <nikita.pavlov@outlook.com>
 */

#include "saadc.h"
#include "application_config.h"
#include "lcd_st7735.h"
#include "nrf_drv_saadc.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf_log.h"

static nrf_saadc_value_t m_buffer[SAMPLES_IN_BUFFER];
saadc_config_t adc_reading;

static bool m_saadc_initialized = false;
static volatile bool m_sampling = false;

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

    adc_reading.adc = p_event->data.done.p_buffer[0];

    m_sampling = false;
  }
}

ret_code_t saadc_init(void) {
  ret_code_t err_code = NRF_SUCCESS;

  nrf_saadc_channel_config_t channel_config =
      NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1); // Pin P0.03

  channel_config.reference = NRF_SAADC_REFERENCE_VDD4;
  channel_config.gain = SAADC_CH_CONFIG_GAIN_Gain1_6;
  channel_config.acq_time = NRF_SAADC_ACQTIME_3US;

  err_code |= nrf_drv_saadc_init(&saadc_config, saadc_callback);

  err_code |= nrf_drv_saadc_channel_init(0, &channel_config);

  err_code |= nrf_drv_saadc_buffer_convert(m_buffer, SAMPLES_IN_BUFFER);

  return err_code;
}

ret_code_t saadc_uninit(void) {
  nrf_drv_saadc_uninit();

  return NRF_SUCCESS;
}

ret_code_t saadc_sample(void) {
  ret_code_t error_code = NRF_SUCCESS;
  m_sampling = true;

  error_code = nrf_drv_saadc_sample();

  return error_code;
}

void saadc_init_sample_uninit(int16_t adc) {
  nrf_gpio_pin_set(TBASE_SET_PIN);
  saadc_init();
  saadc_sample();
  adc = adc_reading.adc;
  screen_clear();
  moisture_print(adc);
  NRF_LOG_INFO("ADC: %d SAADC: %d", adc, adc_reading.adc);
  while (m_sampling == true)
    ;
  saadc_uninit();
  nrf_gpio_pin_clear(TBASE_SET_PIN);
}