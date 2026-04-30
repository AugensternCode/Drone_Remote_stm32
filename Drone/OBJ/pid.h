#ifndef _pid_h_
#define _pid_h_

#include "stm32f10x.h"

/*
 * PID 控制器数据结构。
 *
 * Pid 保存单个 PID 控制器的参数和运行状态。
 * AllPid 集中保存姿态、角速度、高度和定点控制用到的 PID。
 */

typedef struct
{
    float err;              /* 当前误差 */
    float err_last;         /* 上一次误差 */
    float expect;           /* 期望值 */
    float feedback;         /* 反馈值 */
    float kp;               /* 比例系数 */
    float ki;               /* 积分系数 */
    float kd;               /* 微分系数 */
    float integral;         /* 积分项 */
    float integral_max;     /* 积分限幅 */
    float out;              /* PID 输出 */
    float out_max;          /* 输出限幅 */
} Pid;

typedef struct
{
    Pid pitAngle;           /* pitch 姿态角外环 */
    Pid rolAngle;           /* roll 姿态角外环 */
    Pid yawAngle;           /* yaw 姿态角外环 */
    Pid pitGyro;            /* pitch 角速度内环 */
    Pid rolGyro;            /* roll 角速度内环 */
    Pid yawGyro;            /* yaw 角速度内环 */
    Pid acc_high;           /* 高度加速度环 */
    Pid vel_high;           /* 高度速度环 */
    Pid pos_high;           /* 高度位置环 */
    Pid acc_fix_x;          /* X 方向加速度环 */
    Pid vel_fix_x;          /* X 方向速度环 */
    Pid pos_fix_x;          /* X 方向位置环 */
    Pid acc_fix_y;          /* Y 方向加速度环 */
    Pid vel_fix_y;          /* Y 方向速度环 */
    Pid pos_fix_y;          /* Y 方向位置环 */
} AllPid;

/* 执行一次 PID 运算。 */
float PidController(Pid *controller);

/* 初始化全部 PID 参数，必要时从 Flash 读取保存值。 */
void AllPidInit(void);

/* 清除单个 PID 控制器的积分项。 */
void ClearIntegral(Pid *controller);

#endif
