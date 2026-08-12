/*
 * ?????SysTick ???????
 */
#include "systick.h"

/* 系统滴答定时器 */

uint32_t tickCount;

//void SystickInit(void)
//{
//	SysTick->LOAD = (uint32_t)(SystemCoreClock/1000000 - 1UL); //1s/1000,000=1us
//	SysTick->VAL  = 0UL;
//	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |SysTick_CTRL_TICKINT_Msk;  //配置滴答定时器时钟源和启动定时中断
//	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;                     //失能滴答定时器中断中断
//}
void SystickInit(void)
{
	if(SysTick_Config(SystemCoreClock/1000000U)!=0)
	{
		while(1)
		{
		}
	}
}

//us延时
void delay_us(uint32_t time)
{
	if(time<=0)
		return;
	tickCount = time;
	SysTick->VAL = 0;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;                               //使能滴答定时器中断
	while(tickCount!=0);                                                   //等待计时完成
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;                              //失能滴答定时器中断中断
}
//ms延时
void delay_ms(uint32_t time)
{
	if(time<=0)
		return;
	tickCount = time*1000;
	SysTick->VAL = 0;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;                               //使能滴答定时器中断
	while(tickCount!=0);                                                   //等待计时完成
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;                              //失能滴答定时器中断中断
}
//s延时
void delay_s(uint32_t time)
{
	if(time<=0) return;
	tickCount=time*1000000;
	SysTick->VAL=0;
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk; //使能滴答定时器中断
	while(tickCount!=0);
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk; //使能
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    if(tickCount!=0)
    {
        tickCount--;
    }
}


