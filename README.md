
## Simple SPI ADC imitator at STM32F4DISCOVERY ##

This is a software of imaginary ADC that generates the "Saw" signal and delivers it to a client computer through the SPI module.
The simulation can be used to debug your Linux client intended for communicating with a custom SPI-featured ADC board built on the basis of the STM32 family of microcontrollers. 

#### Hardware: ####

- Master: OS Linux, ARM-based SBC (Orange Pi, Raspberry Pi)	
- Slave : Board STM32F4DISCOVERY

#### Connections: ####

| Discovery Pin| Computer Pin|
| :----------: | :---------: |
|    GND       |    GND      |
|    PA4+GND   |    CS+GND   |
|    PA5       |    CLK      |
|    PA7       |    MOSI     |
|    PA6       |    MISO     |


#### Application stuff: ####

Options: MSB, CPOL = 0, CPHA = 1

Package structure (Master-Slave):

| Command|      Pause       |Command Data|      Pause      |Reading ADC Data (data shift) |
|--------|------------------|------------|-----------------|------------------------------|
| 1 byte |(50-500) microsec.|  2 bytes   |(0-100) microsec.|       0-65536 bytes          |


Package structure (Slave-Master):

|Garbage |      Pause       |Command Reply|      Pause      |   ADC Data          |
|--------|------------------|-------------|-----------------|---------------------|
| 1 byte |(50-500) microsec.|   2 bytes   |(0-100) microsec.| 0-65536 bytes (LSB?)|


Commands:

Command Name         | Command Code|Command Data|Command Reply|ADC Data\*
---------------------|-------------|------------|-------------|-----------
Stop ADC             |    0        |   0xFFFF   |     0       |   0 bytes
Start ADC            |    1        |   0xFFFF   |     1       |   0 bytes
Setup Channel Number |    2        | 1-1 channel, 2-2 channels, 3-3 channels, 4-4 channels|2|0 bytes
Setup Input Range    |    3        |      ?     |     3       |   0 bytes
Setup Sample Size    |    4        |2-2 bytes, 3-3 bytes, 4-4 bytes|4|0 bytes
Setup Sampling Rate  |    5        |0-10000 Hz, 1-20000 Hz, 3-50000 Hz, 4-100000 Hz|5|0 bytes
Receive ADC Data     |    6        |   0xFFFF   |0 - data not ready, or "value" > 0 |converted DATA of "value" bytes, if any

\* The size of the “ADC Data” field shall be a multiple of the value equal to the product of “Channel Number” and “Sample Size”. The condition must be observed so that the received data of the last measurement have values for all channels used. Also, the size of this field should be closer to the value of 65535.


