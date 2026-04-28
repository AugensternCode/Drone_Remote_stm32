#ifndef _timer_h_
#define _timer_h_

#include "stm32f10x.h"

/*
 * 主调度定时器模块。
 *
 * TIM3 以 5ms 周期触发一次更新中断，
 * 是飞控主循环调度的节拍来源。
 */

typedef struct
{
    float last_time_us;   /* 上一次记录的时间戳，单位 us */
    float now_time_us;    /* 当前时间戳，单位 us */
    float delta_time_us;  /* 两次记录之间的时间差，单位 us */
    float delta_time_ms;  /* 两次记录之间的时间差，单位 ms */
} _Time_test;

/* 初始化 TIM3，使其每 5ms 触发一次中断。 */
void TimerInit(void);

/* 根据运行计数器更新时间统计结构。 */
void time_check(_Time_test *running);

#endif
