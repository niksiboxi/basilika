#ifndef SAADC_H
#define SAADC_H

#include "nrf_drv_saadc.h"

ret_code_t saadc_init(void);
ret_code_t saadc_uninit(void);
ret_code_t saadc_sample(void);
void saadc_init_sample_uninit(void);

typedef struct{
  nrf_saadc_value_t adc;
} saadc_config_t;

#endif
