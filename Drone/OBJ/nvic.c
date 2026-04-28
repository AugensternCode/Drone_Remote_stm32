/*
 * ?????NVIC ????????
 */

#include "nvic.h"


void NVIC_config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;    

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);             //优先级组别2

    //飞控任务调度定时器中断
		NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;    
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;               
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            
		NVIC_Init(&NVIC_InitStructure);                            
    
    //串口数据接收中断
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}







