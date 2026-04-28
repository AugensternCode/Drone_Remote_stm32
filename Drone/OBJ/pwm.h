#ifndef _pwm_h_
#define _pwm_h_

#include "stm32f10x.h"

/* PWM ????? */

void PwmInit(void);
void PwmOut(int32_t pwm1, int32_t pwm2, int32_t pwm3, int32_t pwm4);

#endif
