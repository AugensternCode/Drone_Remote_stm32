/*
 * 模块说明：TIM3 主调度定时器实现，是飞控 5ms 主循环节拍来源。
 */
#include "timer.h"
#include "led.h"
#include "systick.h"
#include "iic.h"
#include "mpu6050.h"
#include "imu.h"
#include "usart2.h"
#include "nrf24l01.h"
#include "pwm.h"
#include "adc.h"
#include "controller.h"
#include "parse_packet.h"
#include "fc_status.h"
#include "gyro_cal.h"
#include "acc_cal.h"
#include "pair_freq.h"

extern PlaneData plane;
extern Mpu6050_Data Mpu;
extern Attitude att;

/*
 * 初始化 TIM3。
 * 72MHz 时钟经过 72 分频后得到 1MHz 计数频率，
 * 即计数器每增加 1 表示经过 1us。
 * 自动重装值设为 5000，因此每 5ms 触发一次更新中断。
 */
void TimerInit(void)
{
    TIM_TimeBaseInitTypeDef TIM_timeBaseStucture = {0};
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_timeBaseStucture.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_timeBaseStucture.TIM_Prescaler = 72 - 1;
    TIM_timeBaseStucture.TIM_Period = 5000 - 1;
    TIM_timeBaseStucture.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_timeBaseStucture);
    TIM_Cmd(TIM3, ENABLE);
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
}
/* 记录已经走过的完整 5ms 周期数。 */
uint32_t running_tim_cnt = 0;
/*
 * 更新时间统计信息。
 * running_tim_cnt 记录完整周期数，
 * TIM3->CNT 记录当前周期内已经过去的微秒数。
 * 两者组合后可以得到当前时间戳和两次调用之间的时间差。 
 */
void time_check(_Time_test *running)
{
    running->last_time_us = running->now_time_us;  
    //72 MHz 时钟 → 计数 72,000,000 次 = 1 秒。->计数一次1us,中断一次计数5000次
    running->now_time_us = running_tim_cnt * 5000 + TIM3->CNT;
    running->delta_time_us = running->now_time_us - running->last_time_us;
    running->delta_time_ms = running->delta_time_us * 0.001f;
}

void time_check(_Time_test *runing)
{
    runing->last_time_us=runing->now_time_us;
    runing->now_time_us=running_tim_cnt*5000+TIM3->CNT;
    runing->delta_time_us=runing->now_time_us-runing->last_time_us;
    runing->delta_time_ms=runing->delta_time_us*0.001f;
}

_Time_test run_start;
_Time_test run_stop;
/* 把 200Hz 主循环中的遥测发送下采样到 50Hz。 */
static uint8_t telemetry_div = 0;
/*
 * TIM3 更新中断服务函数。
 * 这是飞控主调度入口，每 5ms 执行一次，主要完成：
 * 1. 对频和无线数据处理。
 * 2. 姿态角更新。
 * 3. 控制器计算与 PWM 输出。
 * 4. 状态灯更新与串口遥测发送。
 */
void TIM3_IRQHandler(void)
{
    if (TIM3->SR & 0X0001) //判断TIM3是否产生了更新中断，SR状态寄存器的bit0为更新中断标志位
    {
        running_tim_cnt++;  //更新中断一次，计数自增
        time_check(&run_start); //测量整个中断控制流程的执行耗时
        wait_pairing();  //等待或处理遥控器/无线模块的配对流程
        NrfACKPacket();  //处理NRF无线模块的ACK数据包
        nrf_parse_packet(); //处理NRF接收到的数据包
        GyroDataTransformDeg(&Mpu.deg_s.x, &Mpu.deg_s.y, &Mpu.deg_s.z); //将陀螺仪原始角速度数据转为角度单位
        IMU(&att.rol, &att.pit, &att.yaw); //姿态解算
        ControModel();  //控制模式处理
        _controller_perform(); //执行控制器运算
        ControllerOutput(); //控制量输出
        PlaneLockStatus(); ///无人机锁定状态检测或更新
        RGB_LedStatus(plane); //无人机 LED状态显示
        if (++telemetry_div >= 4) //遥测数据发送分频 遥测发送频率 = TIM3 中断频率 / 4
        {
            telemetry_div = 0;  //清零分频计数器
            ANOSendStatus();  //给匿名上位机发送状态
        }
        time_check(&run_stop);  //记录或检测本次控制周期的结束时间
    }
    TIM3->SR &= ~(1 << 0); //清除更新中断标志位
}

/*
1.中断更新，进入控制逻辑
2.检查遥控器和无人机的配对状态
3.配对完成遥控器，返回ACK携带信息
4.遥控器给无人机发送NRF数据包，并解析
5.陀螺仪原始数据角速度解析为角度单位？
6.姿态解算+控制模式处理+控制量输出
7.无人机锁定状态更新
8.无人机状态用LED显示
9.给匿名上位机的数据发送状态
*/