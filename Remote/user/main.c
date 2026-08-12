#include "stm32f10x.h"
#include "systick.h"
#include "led.h"
#include "spi.h"
#include "nrf24l01.h"
#include "usart1.h"
#include "timing_trigger.h"
#include "nvic.h"
#include "adc.h"
#include "key.h"
#include "pair_freq.h"
#include "sendpacket.h"
#include "oled.h"
int main(void)
{
    SystemInit();                       //系统初始化    
    systick_init();                     //系统滴答定时器初始化
    LedInit();                         //状态灯初始化
    Usart1Init(115200);                //串口初始化 
    printf("usart is ok\r\n");
	get_chip_id();
    SPI1_Init();
    NRF24L01_Init();
    while(NRF24L01_Check()){
			LedBlink(RED);
			delay_ms(10);
    }
    NRF24L01_TX_Mode();
    KeyInit();
		ADC_Config();
		timing_trigger_init();
		OLED_Init();			//初始化OLED  
		OLED_Clear(); 		
       NVIC_config();                      //中断配置初始化
    delay_ms(100);
    while(1){
			WaitPairing();
			key_info();
			OledDisplayPairStatus();

    }
}
//int main()                              
//{                      
//	
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
////	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
////	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE); 
//	GPIO_InitTypeDef GPIO_InitStruct;
//	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
//	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_13 | GPIO_Pin_14;
//	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
//	GPIO_Init(GPIOB,&GPIO_InitStruct);
//	GPIO_SetBits(GPIOB,GPIO_Pin_13);
//   	GPIO_SetBits(GPIOB,GPIO_Pin_14);
//	while(1)
//	{
//	}
//}