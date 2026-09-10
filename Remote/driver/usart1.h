#ifndef  USART1_H_
#define  USART1_H_
#include "stm32f10x.h"
#include "stdio.h"

#define TX_GPIO_PORT GPIOA
#define TX_GPIO_PIN GPIO_Pin_9
#define TX_GPIO_CLK RCC_APB2Periph_GPIOA
#define USART_TX_GPIO_CLKCMD RCC_APB2PeriphClockCmd

#define RX_GPIO_PORT GPIOA
#define RX_GPIO_PIN GPIO_Pin_10
#define RX_GPIO_CLK RCC_APB2Periph_GPIOA
#define USART_RX_GPIO_CLKCMD RCC_APB2PeriphClockCmd

#define USARTx  USART1
#define USARTx_CLK RCC_APB2Periph_USART1
#define USARTx_CLKCMD RCC_APB2PeriphClockCmd

void Usart1Init(u32 bound);
//void USART2_DMA_SEND_DATA(u32 SendBuff,u16 len);
//void ANO_DMA_SEND_DATA(void);
#endif
