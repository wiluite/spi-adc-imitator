#ifndef SPI_H
#define SPI_H

#include <stdint.h>
typedef int32_t recv_sample_type; 

#define ADC_BUF_SIZE 64000
static int systick_times; // = 1; unnecessary initialization

void SPI_ini(void);
void SPI_DMA_ini(void);
void SPI_CMD_QUERY(void);
void SPI_CMD_REPLY(void);
void SPI_ARRAY(void);

#endif
