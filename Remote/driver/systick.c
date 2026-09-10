#include "systick.h"

/* 系统滴答定时器 */
__IO uint32_t g_system_tick_ms=0U;

void SysTick_init(void)
{
	if(SysTick_Config(SystemCoreClock/1000U)!=0U) while(1);
}

uint32_t GetTick(void)
{
	return g_system_tick_ms;
}
//ms延时
void Delay_ms(uint32_t ms)
{
	uint32_t start=GetTick();
	while((GetTick()-start)<ms);
}
//us延时
void Delay_us(uint32_t us)
{
   uint32_t start=SysTick->VAL;
   uint32_t tick=us*72;   //72个时钟周期计数1us
    while(((start-SysTick->VAL)&0x00FFFFFF)<tick);  //防溢出
}
