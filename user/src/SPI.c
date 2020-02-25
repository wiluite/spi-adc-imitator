#include "common.h"
#include "SPI.h"

/*
Standard approach to setup SPI (SPI1)
*/
void SPI_ini(void)
{
    GPIO_InitTypeDef GPIO_Init_LED;
    SPI_InitTypeDef SPI_Init_user;

    // turn on clocking/timing of the corresponding SPI bus
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    // turn on clocking/timing of the GPIO port A bus
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    // setup MOSI, MISO, CLK pins of the SPI1------------
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

    GPIO_Init_LED.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
    GPIO_Init_LED.GPIO_Mode = GPIO_Mode_AF;
    GPIO_Init_LED.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init_LED.GPIO_OType = GPIO_OType_PP;
    GPIO_Init_LED.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_Init(GPIOA, &GPIO_Init_LED);
    //---------------------------------------------------

    // setup SPI1 periphery
    SPI_Init_user.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_Init_user.SPI_Mode = SPI_Mode_Slave;
    SPI_Init_user.SPI_DataSize = SPI_DataSize_8b;
    SPI_Init_user.SPI_CPOL = SPI_CPOL_Low;
    SPI_Init_user.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_Init_user.SPI_NSS = SPI_NSS_Soft;
    SPI_Init_user.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_Init_user.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init_user.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &SPI_Init_user);

    SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Reset);

    // enable SPI functionality
    SPI_Cmd(SPI1, ENABLE);
}


__IO uint8_t SPI_work1=0;
__IO uint8_t SPI_work2=0;

inline void StartSPI (uint16_t number)
{
    DMA_SetCurrDataCounter (DMA2_Stream0, number);
    DMA_SetCurrDataCounter (DMA2_Stream3, number);
    SPI_work1 = 1, SPI_work2 = 1;
    DMA_Cmd (DMA2_Stream0, ENABLE);
    DMA_Cmd (DMA2_Stream3, ENABLE);
}


extern uint16_t __IO sys_tick_calls;

//#define SHOULD_CHECK_TIMEOUT

#if defined(SHOULD_CHECK_TIMEOUT)
const uint16_t SYSTICK_TIMES_10 = 200 /*some other number!*/; 
inline uint8_t SPI_TIMEOUT_10SEC()
{
    return (sys_tick_calls > SYSTICK_TIMES_10) ? 1 : 0;
}
#endif

void reboot_if_timeout(void)
{
#if defined(SHOULD_CHECK_TIMEOUT)
    if (SPI_TIMEOUT_10SEC())
    {
        NVIC_SystemReset();
    }
#endif
}

inline void SPIwait(void)
{
    sys_tick_calls = 0;
    while (SPI_work1 || SPI_work2)
    {
        reboot_if_timeout();
    }
}

uint8_t spi_array_flag = 0;
__align(4) uint8_t adc_buf1[ADC_BUF_SIZE];
__align(4) uint8_t adc_buf2[ADC_BUF_SIZE];

inline void SPIwaitArr(void)
{
    sys_tick_calls = 0;
    uint16_t fill_buff_count = 0;
    uint8_t * const arr_to_fill = (spi_array_flag) ? adc_buf1 : adc_buf2;
		
    while (SPI_work1 || SPI_work2)
    {
        reboot_if_timeout();

        if (fill_buff_count < ADC_BUF_SIZE)
        {
            *(recv_sample_type*)(&arr_to_fill[fill_buff_count]) = fill_buff_count / sizeof(recv_sample_type);
            fill_buff_count += sizeof(recv_sample_type);
        } 
    }

    if (ADC_BUF_SIZE == fill_buff_count)
    {	
       GPIO_SetBits (GPIOD, GPIO_Pin_12); // green
    }
    else
    {
       GPIO_SetBits (GPIOD, GPIO_Pin_13); // orange
    }
}

//-----------------------------------------------------------------
//RX
void DMA2_Stream0_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0) == SET)
    {
        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
        SPI_work1=0;
    }
}

//-----------------------------------------------------------------
//TX
void DMA2_Stream3_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA2_Stream3, DMA_IT_TCIF3) == SET)
    {
        DMA_ClearITPendingBit(DMA2_Stream3, DMA_IT_TCIF3);
        SPI_work2=0;
    }
}

DMA_InitTypeDef DMA_Init_SPI;

void GLOBAL_DMA_init(void)
{
    DMA_Init_SPI.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
    DMA_Init_SPI.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_Init_SPI.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_Init_SPI.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_Init_SPI.DMA_Mode = DMA_Mode_Normal;
    DMA_Init_SPI.DMA_Priority = DMA_Priority_Medium;
    DMA_Init_SPI.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_Init_SPI.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
    DMA_Init_SPI.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_Init_SPI.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
}

inline void REINIT_TX(uint32_t buf_sz, uint32_t base_addr)
{
    DMA_ITConfig(DMA2_Stream3, DMA_IT_TC, DISABLE);

    DMA_DeInit (DMA2_Stream3);

    DMA_Init_SPI.DMA_Channel = DMA_Channel_3;
    DMA_Init_SPI.DMA_Memory0BaseAddr = (uint32_t)base_addr;
    DMA_Init_SPI.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_Init_SPI.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_Init_SPI.DMA_BufferSize = buf_sz;

    DMA_Init(DMA2_Stream3, &DMA_Init_SPI);

    DMA_ITConfig(DMA2_Stream3, DMA_IT_TC, ENABLE);
}


enum rx_inc_flag {rx_inc_enable, rx_inc_disable};
inline void REINIT_RX(uint32_t buf_sz, uint32_t base_addr, enum rx_inc_flag inc_flag)
{
    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, DISABLE);

    DMA_DeInit(DMA2_Stream0);

    DMA_Init_SPI.DMA_Channel = DMA_Channel_3;
    DMA_Init_SPI.DMA_Memory0BaseAddr = (uint32_t)base_addr;
    DMA_Init_SPI.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_Init_SPI.DMA_MemoryInc = (inc_flag == rx_inc_enable) ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable;
    DMA_Init_SPI.DMA_BufferSize = buf_sz;

    DMA_Init(DMA2_Stream0, &DMA_Init_SPI);

    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
}

void SPI_DMA_ini()
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    GLOBAL_DMA_init();

    //RX
    NVIC_DisableIRQ(DMA2_Stream0_IRQn);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);


    //TX
    NVIC_DisableIRQ(DMA2_Stream3_IRQn);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
}

// to link inline version
void REINIT_RX(uint32_t buf_sz, uint32_t base_addr, enum rx_inc_flag inc_flag);
void REINIT_TX(uint32_t buf_sz, uint32_t base_addr);
void StartSPI (uint16_t number);
void SPIwait(void);
void SPIwaitArr(void);

__align(4) uint8_t Cmd_SPI_In[2] = {0, 0, };
__align(4) uint8_t Cmd_SPI_Out[2] = {0, 0, };


void SPI_CMD_QUERY(void)
{
    REINIT_RX(1, (uint32_t)Cmd_SPI_In, rx_inc_enable);
    REINIT_TX(1, (uint32_t)Cmd_SPI_Out);
    StartSPI(1);
    SPIwait();
}

void SPI_CMD_REPLY(void)
{
    REINIT_RX(2, (uint32_t)Cmd_SPI_In, rx_inc_enable);
    REINIT_TX(2, (uint32_t)Cmd_SPI_Out);
    StartSPI(2);
    SPIwait();
}


__align(4) uint8_t Dummy_SPI_In[1];

extern uint16_t get_shift_size(void);

inline void SPI_ARRAY(void)
{
    REINIT_RX(get_shift_size(), (uint32_t)Dummy_SPI_In, rx_inc_disable);
    REINIT_TX(get_shift_size(), (spi_array_flag ? (uint32_t)adc_buf2 : (uint32_t)adc_buf1 ));

    StartSPI(get_shift_size());

    SPIwaitArr();
	
    spi_array_flag = (spi_array_flag) ? 0 : 1;
    return;
}

