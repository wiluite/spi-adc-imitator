#include "common.h"
#include "SPI.h"
#include "sm.h"


extern __IO uint8_t data_ready;
uint16_t __IO sys_tick_calls = 0;

void SysTick_Handler(void)
{
    if (data_ready == 1)
    {
       GPIO_SetBits (GPIOD, GPIO_Pin_15);
    } else
    {
       data_ready = 1;
    }	
    ++sys_tick_calls;
}

extern uint8_t adc_buf1[ADC_BUF_SIZE];

int main(void)
{
    SystemInit();
    GPIO_InitTypeDef init;

    init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    init.GPIO_Mode = GPIO_Mode_OUT;
    init.GPIO_OType = GPIO_OType_PP;
    init.GPIO_Speed = GPIO_Low_Speed;
    init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOD, ENABLE);

    SystemCoreClockUpdate();

    GPIO_Init (GPIOD, &init);

    uint16_t i = 0;
    while (i < ADC_BUF_SIZE)
    {
        // This is because the first buffer should be already prepared for outcome
        *(recv_sample_type*)(&adc_buf1[i]) = i / sizeof(recv_sample_type);
        i += sizeof(recv_sample_type);
    }

    SPI_ini();
    SPI_DMA_ini();

    process();

    // critical state - turn on red lamp
    GPIO_SetBits (GPIOD, GPIO_Pin_14);
}
