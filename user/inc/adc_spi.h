#ifndef ADC_SPI
#define ADC_SPI

#include <stdint.h>

uint16_t adc_cmd0_callback(void);

uint16_t adc_cmd1_callback(void);


// channel number
uint16_t adc_cmd2_callback(void);
void adc_cmd2_data_callback(uint16_t data);

// input range
uint16_t adc_cmd3_callback(void);
void adc_cmd3_data_callback(uint16_t data);

// adc sample size
uint16_t adc_cmd4_callback(void);
void adc_cmd4_data_callback(uint16_t data);

// adc sampling rate
uint16_t adc_cmd5_callback(void);
void adc_cmd5_data_callback(uint16_t data);

// adc buffer size
uint16_t adc_cmd6_callback(void);


#endif

