/*
 * ?????TIM1 ?? PWM ???????????????/?????
 */
#include "pwm.h"
#include "imath.h"
#include "precompile.h"

#if FOUR_AXIS_UAV
static const uint16_t arrValue = 1000 - 1;
static const uint16_t pscValue = 8 - 1;
static const uint16_t ccrValue = 0;
#elif FIXED_WING_AIRCRAFT
static const uint16_t arrValue = 1000 - 1;
static const uint16_t pscValue = 8 - 1;
static const uint16_t ccrValue = 0;
#elif BRUSHLESS_FOUR_AXIS_UAV
static const uint16_t arrValue = 10000 - 1;
static const uint16_t pscValue = 16 - 1;
static const uint16_t ccrValue = 4000;
#endif

// TIM1 四路 PWM 输出初始化。
// 不同机型使用不同的 ARR/PSC/默认占空比，以适配空心杯和无刷电调。
void PwmInit(void)
{
	GPIO_InitTypeDef GPIO_initStructure = {0};
	TIM_TimeBaseInitTypeDef TIM_timeBaseInitStructure = {0};
	TIM_OCInitTypeDef TIM_OCInitStructure = {0};

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);

	GPIO_initStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_initStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_initStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_initStructure);

	// 定时器周期: t = (ARR + 1) * (PSC + 1) / T_clock
	TIM_timeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_timeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_timeBaseInitStructure.TIM_Period = arrValue;
	TIM_timeBaseInitStructure.TIM_Prescaler = pscValue;
	TIM_TimeBaseInit(TIM1,&TIM_timeBaseInitStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = ccrValue;

	TIM_OC1Init(TIM1,&TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM1,TIM_OCPreload_Enable);

	TIM_OC2Init(TIM1,&TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM1,TIM_OCPreload_Enable);

	TIM_OC3Init(TIM1,&TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM1,TIM_OCPreload_Enable);

	TIM_OC4Init(TIM1,&TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM1,TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM1,ENABLE);
	TIM_Cmd(TIM1,ENABLE);
	TIM_CtrlPWMOutputs(TIM1,ENABLE);
}

// 输出前统一做限幅，防止控制量为负或过大时直接写入 CCR。
void PwmOut(int32_t pwm1,int32_t pwm2,int32_t pwm3,int32_t pwm4)
{
#if FOUR_AXIS_UAV
	TIM1->CCR1 = (uint16_t)LimitInt32(pwm1,0,1000);
	TIM1->CCR2 = (uint16_t)LimitInt32(pwm2,0,1000);
	TIM1->CCR3 = (uint16_t)LimitInt32(pwm3,0,1000);
	TIM1->CCR4 = (uint16_t)LimitInt32(pwm4,0,1000);
#elif FIXED_WING_AIRCRAFT
	TIM1->CCR1 = (uint16_t)LimitInt32(pwm1,0,1000);
	TIM1->CCR2 = (uint16_t)LimitInt32(pwm2,0,1000);
	TIM1->CCR3 = (uint16_t)LimitInt32(pwm3,0,1000);
	TIM1->CCR4 = (uint16_t)LimitInt32(pwm4,0,1000);
#elif BRUSHLESS_FOUR_AXIS_UAV
	TIM1->CCR1 = (uint16_t)LimitInt32(pwm1,0,10000);
	TIM1->CCR2 = (uint16_t)LimitInt32(pwm2,0,10000);
	TIM1->CCR3 = (uint16_t)LimitInt32(pwm3,0,10000);
	TIM1->CCR4 = (uint16_t)LimitInt32(pwm4,0,10000);
#endif
}
