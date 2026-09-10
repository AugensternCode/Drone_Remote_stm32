#ifndef _systick_h_
#define _systick_h_
#include "stm32f10x.h"
void SysTick_init(void);
void Delay_us(uint32_t us);
uint32_t GetTick(void);
void Delay_ms(uint32_t ms);
#endif
