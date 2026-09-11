#ifndef _SPI_H_
#define _SPI_H_
#include "stm32f10x.h"
#define RCC_SPI1_CLK       RCC_APB2Periph_SPI1
#define SPI1_GPIO_PORT     GPIOA
#define RCC_SPI1_PORT      RCC_APB2Periph_GPIOA
#define SPI1_SCK           GPIO_Pin_5
#define SPI1_MISO          GPIO_Pin_6
#define SPI1_MOSI          GPIO_Pin_7
#define SPI1_SCK_SOURCE    GPIO_PinSource5
#define SPI1_MISO_SOURCE   GPIO_PinSource6
#define SPI1_MOSI_SOURCE   GPIO_PinSource7

void SPI1_Init(void);			 
uint8_t SPI1_RW(uint16_t TxData);
#endif
