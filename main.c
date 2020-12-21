#include "app_error.h"
#include "application_config.h"
#include "lcd_st7735.h"
#include "nordic_common.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "rtc.h"
#include "saadc.h"
#include "sdk_config.h"
#include <stdlib.h>
#include <inttypes.h>
#include "nrf_drv_twi.h"

static volatile bool m_sampling = false;

static const nrf_drv_twi_t m_twi_master = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE);

#define TSL2561_ADDR          0x39
#define TSL2561_CMD           0x80
#define TSL2561_CMD_CLEAR     0xC0
#define	TSL2561_REG_CONTROL   0x00
#define	TSL2561_REG_TIMING    0x01
#define	TSL2561_REG_THRESH_L  0x02
#define	TSL2561_REG_THRESH_H  0x04
#define	TSL2561_REG_INTCTL    0x06
#define	TSL2561_REG_ID        0x0A
#define	TSL2561_REG_DATA_0    0x0C
#define	TSL2561_REG_DATA_1    0x0E

static saadc_config_t saadc;

static void lfclk_config(void) {
  APP_ERROR_CHECK(nrf_drv_clock_init());

  nrf_drv_clock_lfclk_request(NULL);
}

static ret_code_t twi_master_init(void)
{
  ret_code_t err_code = NRF_SUCCESS;

  const nrf_drv_twi_config_t config = 
  {
    .scl = TWI_SCL_PIN,
    .sda = TWI_SDA_PIN,
    .frequency = NRF_DRV_TWI_FREQ_400K,
    .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
    .clear_bus_init = false
  };

  err_code |= nrf_drv_twi_init(&m_twi_master, &config, NULL, NULL);

  return err_code;
}

ret_code_t twi_send(uint8_t address, uint8_t * p_data)
{
  ret_code_t err_code = NRF_SUCCESS;

  err_code |= nrf_drv_twi_tx(&m_twi_master, address, p_data, sizeof(p_data), false);

  return err_code;
}

ret_code_t twi_read(uint8_t address, uint8_t * p_data)
{
  ret_code_t err_code = NRF_SUCCESS;

  err_code |= nrf_drv_twi_rx(&m_twi_master, address, p_data, sizeof(p_data));

  return err_code;
}

ret_code_t tsl2561_init(void)
{
  ret_code_t err_code = NRF_SUCCESS;
  uint8_t powerup = 0x03;
  uint8_t id;

  err_code |= twi_send(TSL2561_ADDR, &powerup); //<- Power Up the TSL2561

  return err_code;
}

ret_code_t tsl2561_shutdown(void)
{ 
  ret_code_t err_code = NRF_SUCCESS;
  uint8_t shutdown = 0x00;

  err_code |= twi_send(TSL2561_ADDR, &shutdown);

  return err_code;
}

ret_code_t tsl2561_setTiming(void)
{
  ret_code_t err_code = NRF_SUCCESS;

  uint8_t timing = 0x02;
  uint8_t read_timing;

  err_code |= twi_send(TSL2561_REG_TIMING, &timing);
  err_code |= twi_read(TSL2561_REG_TIMING, &read_timing);

  NRF_LOG_INFO("Timing: %" PRIu8 "", read_timing);

  return err_code;
}

ret_code_t tsl2561_getData(void)
{
  ret_code_t err_code = NRF_SUCCESS;
  uint8_t data0, data1;

  err_code |= twi_read(TSL2561_REG_DATA_0, &data0);
  err_code |= twi_read(TSL2561_REG_DATA_1, &data1);

  NRF_LOG_INFO("DATA0: %" PRIu8 " DATA1: %" PRIu8 "", data0, data1);

  nrf_delay_ms(1000);

  return err_code;
}

ret_code_t tsl2561_getId(void)
{
  ret_code_t err_code = NRF_SUCCESS;
  uint8_t id;

  err_code |= twi_read(TSL2561_REG_ID, &id);

  NRF_LOG_INFO("TSL2561 ID: %" PRIu8 "", id);

  return err_code;
}

int main(void) {
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

  NRF_LOG_DEFAULT_BACKENDS_INIT();

  nrf_gpio_cfg_output(TBASE_SET_PIN);

  gfx_init();
  background_set();
  screen_clear();
  lfclk_config();
  rtc_config();
  twi_master_init();
  tsl2561_getId();
  tsl2561_setTiming();
  tsl2561_init();  

  while (1) {
    tsl2561_getData();
    // Make sure any pending events are cleared
    __SEV();
    __WFE();
    // Enter System ON sleep mode
    __WFE();
  }
}