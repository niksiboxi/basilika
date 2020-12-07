#include "app_error.h"
#include "application_config.h"
#include "boards.h"
#include "nordic_common.h"
#include "nrf_drv_spi.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "rtc.h"
#include "saadc.h"
#include "sdk_config.h"
#include <string.h>

#define SPI_INSTANCE 0
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);
static volatile bool spi_xfer_done;

static volatile bool m_sampling = false;

void spi_evt_handler(nrf_drv_spi_evt_t const * p_event,
                     void *                    p_context)
{
  spi_xfer_done = true;  
}

static void lfclk_config(void) {
  APP_ERROR_CHECK(nrf_drv_clock_init());

  nrf_drv_clock_lfclk_request(NULL);
}

void spi_init(void)
{
  nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
  spi_config.ss_pin = 5;
  spi_config.miso_pin = 8;
  spi_config.mosi_pin = 6;
  spi_config.sck_pin = 7;
  APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_evt_handler, NULL));
}

int main(void) {
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

  NRF_LOG_DEFAULT_BACKENDS_INIT();

  nrf_gpio_cfg_output(TBASE_SET_PIN);
  
  nrf_gpio_cfg(TFT_LITE,
               NRF_GPIO_PIN_DIR_OUTPUT,
               NRF_GPIO_PIN_INPUT_DISCONNECT,
               NRF_GPIO_PIN_NOPULL,
               NRF_GPIO_PIN_S0H1,
               NRF_GPIO_PIN_NOSENSE);

  nrf_gpio_pin_set(TFT_LITE);

  lfclk_config();
  rtc_config();
  saadc_init_sample_uninit();
  spi_init();

  NRF_LOG_INFO("Initialized...");

  while (1) {
    // Make sure any pending events are cleared
    __SEV();
    __WFE();
    // Enter System ON sleep mode
    __WFE();
  }
}