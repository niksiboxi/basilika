#ifndef SAADC_H
#define SAADC_H

ret_code_t saadc_init(void);
ret_code_t saadc_uninit(void);
ret_code_t saadc_sample(void);
void saadc_init_sample_uninit(void);

#endif
