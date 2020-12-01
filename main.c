#include "app_error.h"
#include "application_config.h"
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

// SoftDevice
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_advdata.h"
#include "nrfx_glue.h"

#include <string.h>

#define APP_BLE_CONN_CFG_TAG 1
#define NON_CONNECTABLE_ADV_INTERVAL MSEC_TO_UNITS(100, UNIT_0_625_MS)

#define APP_BEACON_INFO_LENGTH  0x17  // Total Lenght of advertising information
#define APP_ADV_DATA_LENGTH     0x15  // Manufacturer specific data lentgth
#define APP_DEVICE_TYPE         0x02  // Beacon
#define APP_COMPANY_IDENTIFIER  0xFFFF
#define APP_MAJOR_VALUE         0x01, 0x02
#define APP_MINOR_VALUE         0x03, 0x04
#define APP_BEACON_UUID         0x01, 0x12, 0x23, 0x34, \
                                0x45, 0x56, 0x67, 0x78, \
                                0x89, 0x9a, 0xab, 0xbc, \
                                0xcd, 0xde, 0xef, 0xf0            /**< Proprietary UUID for Beacon. */

#define DEAD_BEEF               0xDEADBEEF

static ble_gap_adv_params_t m_adv_params;
static uint8_t              m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static uint8_t              m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];

/** @brief Struct containing pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data = 
{
  .adv_data = 
  {
    .p_data = m_enc_advdata,
    .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
  },
  .scan_rsp_data =
  {
    .p_data = NULL,
    .len    = 0
  }
};

/** @brief Information advertised by the Beacon. */
static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH] = 
{
  APP_DEVICE_TYPE,
  APP_ADV_DATA_LENGTH,
  APP_BEACON_UUID,
  APP_MAJOR_VALUE,
  APP_MINOR_VALUE
};

void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
  app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void advertising_init(void)
{
  ret_code_t    err_code;
  ble_advdata_t advdata;
  uint8_t       flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

  ble_advdata_manuf_data_t manuf_specific_data;

  memset(&advdata, 0, sizeof(advdata));

  advdata.name_type = BLE_ADVDATA_NO_NAME;
  advdata.flags     = flags;
  advdata.p_manuf_specific_data = &manuf_specific_data;

  manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;

  manuf_specific_data.data.p_data = (uint8_t *) m_beacon_info;
  manuf_specific_data.data.size = APP_BEACON_INFO_LENGTH;

  memset(&m_adv_params, 0, sizeof(m_adv_params));

  m_adv_params.properties.type  = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
  m_adv_params.p_peer_addr      = NULL;
  m_adv_params.filter_policy    = BLE_GAP_ADV_FP_ANY;
  m_adv_params.interval         = NON_CONNECTABLE_ADV_INTERVAL;
  m_adv_params.duration         = 0;

  err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
  APP_ERROR_CHECK(err_code);

  err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
  APP_ERROR_CHECK(err_code);
}

static void advertising_start(void)
{
  ret_code_t err_code;

  err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
  APP_ERROR_CHECK(err_code);
}

static void ble_stack_init(void)
{
  ret_code_t err_code;

  err_code = nrf_sdh_enable_request();
  APP_ERROR_CHECK(err_code);

  uint32_t ram_start = 0;
  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  APP_ERROR_CHECK(err_code);

  // Enable BLE Stack
  err_code = nrf_sdh_ble_enable(&ram_start);
  APP_ERROR_CHECK(err_code);
}

static volatile bool m_sampling = false;

static void lfclk_config(void) {
  if(!nrf_drv_clock_init_check())
    APP_ERROR_CHECK(nrf_drv_clock_init());

  nrf_drv_clock_lfclk_request(NULL);
}

int main(void) {
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

  NRF_LOG_DEFAULT_BACKENDS_INIT();

  nrf_gpio_cfg_output(TBASE_SET_PIN);

  ble_stack_init();
  advertising_init();
  lfclk_config();
  rtc_config();
  saadc_init_sample_uninit();

  NRF_LOG_INFO("Initialized...");

  advertising_start();

  while (1) {
    // Make sure any pending events are cleared
    __SEV();
    __WFE();
    // Enter System ON sleep mode
    __WFE();
  }
}