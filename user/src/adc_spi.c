#include "common.h"
#include "adc_spi.h"
#include "SPI.h"
#include <math.h>

static uint32_t adc_rates [4] = {10000, 20000, 50000, 100000};


// stop adc
uint16_t adc_cmd0_callback(void)
{
    // process it....
    return 0; // reply to Orange
}

//start adc
uint16_t adc_cmd1_callback(void)
{
    // process it....
    return 1; // reply to Orange
}

//-------------------------------
// channel number
uint16_t adc_cmd2_callback(void)
{
    // process it....
    return 2; // reply to Orange
}
uint16_t channel_number = 0;
void recalc_shift_size(void);
void adc_cmd2_data_callback(uint16_t data) // channel number
{
    channel_number = data;
    recalc_shift_size();
}
//-------------------------------
// input range
uint16_t adc_cmd3_callback(void)
{
    // process it....
    return 3;	// reply to Orange
}
uint16_t input_range_code = 0;
void adc_cmd3_data_callback(uint16_t data) // input range
{
    input_range_code = data;
}
//-------------------------------
// adc sample size
uint16_t adc_cmd4_callback(void)
{
    return 4; // reply to Orange
}
uint16_t sample_size = 0;
void adc_cmd4_data_callback(uint16_t data) // adc sample size
{
    sample_size = data;
    recalc_shift_size();
}
//-------------------------------
// adc sampling rate
uint16_t adc_cmd5_callback(void)
{
    return 5; // reply to Orange
}
uint16_t sampling_rate_code = 0;
void adc_cmd5_data_callback(uint16_t data) // adc sampling rate
{
    sampling_rate_code = data;
    recalc_shift_size();
}

//-------------------------------
//

static uint16_t shift_size;
void recalc_shift_size(void)
{
    shift_size = ((uint16_t)ADC_BUF_SIZE / channel_number / sample_size) * channel_number * sample_size;
    if (sampling_rate_code < sizeof(adc_rates)/sizeof(adc_rates[0]))
    {
        systick_times = round(((float)adc_rates[sampling_rate_code] * channel_number * sample_size) / ADC_BUF_SIZE);
    }
    SysTick_Config(SystemCoreClock/systick_times);
}

uint16_t get_shift_size(void)
{
    return shift_size;
}

__IO uint8_t data_ready = 0;
uint8_t data_ready_flag = 0;

//-------------------------------
// adc buffer size
uint16_t adc_cmd6_callback(void)
{
    return ((data_ready_flag = data_ready) == 1) ? get_shift_size() : 0;
}
//-------------------------------



