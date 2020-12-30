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
static int16_t moisture_min = 0;
static int16_t moisture_max = 0;
static float moisture_percentage;

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

  nrf_drv_saadc_config_t saadc_config;
  nrf_saadc_channel_config_t channel_config;

  // Configure SAADC
  saadc_config.low_power_mode = true;                       //<-Enable low-power
  saadc_config.resolution = NRF_SAADC_RESOLUTION_8BIT;      //<-SAADC output value 0 (0V) to 256 (3.6V @ gain setting 1/6)
  saadc_config.oversample = NRF_SAADC_OVERSAMPLE_DISABLED;  //<-Bypass oversampling (Set for high resolution)
  saadc_config.interrupt_priority = APP_IRQ_PRIORITY_HIGH;  //<-Set SAADC interrupt to high priority

  // Initialize SAADC
  err_code |= nrf_drv_saadc_init(&saadc_config, saadc_callback);

  // Configure SAADC channel
  channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;  //<-Set internal reference of fixed 0.6V
  channel_config.gain = SAADC_CH_CONFIG_GAIN_Gain1_6;       //<-SAADC maximum input voltage is 0.6V/(1/6)=3.6V. The single ended input range is the 0V.3.6V
  channel_config.acq_time = NRF_SAADC_ACQTIME_10US;         //<-Low acquisition time to enable maximum sampling frequency of 200kHz. High to allow maximum source resistance up to 800 kohm
  channel_config.pin_p = NRF_SAADC_INPUT_AIN1;              //<-Pin P0.03
  channel_config.pin_n = NRF_SAADC_INPUT_DISABLED;          //<-SAADC is single ended: negative pin is disabled (shorted to ground internally)
  channel_config.resistor_p = NRF_SAADC_RESISTOR_DISABLED;  //<-Disable pullup resistor on the input pin
  channel_config.resistor_n = NRF_SAADC_RESISTOR_DISABLED;  //<-Disable pulldown resistor on the input pin

  // Initialize SAADC channel  
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

  error_code |= nrf_drv_saadc_sample();  //<-Trigger the SAADC SAMPLE task

  return error_code;
}

void saadc_init_sample_uninit(void) {
  int16_t voltage;
  nrf_gpio_pin_set(TBASE_SET_PIN);
  saadc_init();
  saadc_sample();
  screen_clear();

  if(adc_reading.adc > moisture_min){
    if(adc_reading.adc >= moisture_max){
          moisture_max = adc_reading.adc;
        }
    }
  else
  {
    if(adc_reading.adc < moisture_min){
      moisture_min = adc_reading.adc;
    }
  }

  moisture_percentage = (float)(adc_reading.adc - moisture_min) / (moisture_max - moisture_min) * 100;

  moisture_print(moisture_percentage);
  voltage = adc_reading.adc * SAADC_MVOLTS_MULTIPLIER; //<- Ref. SAADC Digital Output
  //NRF_LOG_INFO("ADC: %d Moisture: "NRF_LOG_FLOAT_MARKER" Voltage: %dmV", adc_reading.adc, NRF_LOG_FLOAT(moisture_percentage), voltage);
  while (m_sampling == true)
    ;
  saadc_uninit();
  nrf_gpio_pin_clear(TBASE_SET_PIN);
}