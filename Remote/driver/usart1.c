#include "usart1.h"
//禁用半主机模式
#pragma import(__use_no_semihosting)             
//在 ARM 架构（比如 Keil MDK）中，默认情况下，C 标准库的 printf 等函数会试图通过
//仿真器（JTAG/SWD）在电脑的控制台输出。这叫“半主机模式   
//如果拔掉仿真器会报错
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//不用半主机模式后，链接器会找不到一些底层函数，    
void _sys_exit(int x) 
{ 
	(void)x;
}
//重定向--printf 实际上是一个宏，它最终会逐个字符地调用 fputc 函数把数据打印出去。    
int fputc(int ch, FILE *f)
{      
	while(USART_GetFlagStatus(USARTx,USART_FLAG_TC)==RESET);
	USART_SendData(USARTx,(uint8_t)ch);
	return ch;
}

//上外上位机串口初始化
void Usart1Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	//开启时钟
	USART_TX_GPIO_CLKCMD(TX_GPIO_CLK,ENABLE);
	USART_RX_GPIO_CLKCMD(RX_GPIO_CLK,ENABLE);
	USARTx_CLKCMD(USARTx_CLK,ENABLE);
    //  TX
	GPIO_InitStructure.GPIO_Pin = TX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(TX_GPIO_PORT, &GPIO_InitStructure);
    //  RX
	GPIO_InitStructure.GPIO_Pin = RX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(RX_GPIO_PORT, &GPIO_InitStructure);
	//串口
	USART_InitStructure.USART_BaudRate = bound; 
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     //8bits
	USART_InitStructure.USART_StopBits = USART_StopBits_1;                          //stop bit is 1
	USART_InitStructure.USART_Parity = USART_Parity_No;                             //no parity
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //no Hardware Flow Control
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                 //enable tx and rx
	USART_Init(USARTx, &USART_InitStructure);
	USART_ITConfig(USARTx,USART_IT_RXNE,ENABLE);                                    //rx interrupt is enable
	USART_Cmd(USARTx, ENABLE);    
}
