#ifndef _pid_h_
#define _pid_h_

#include "stm32f10x.h"

/*
 * PID ??????
 *
 * Pid ????????
 * AllPid ???????????????????????
 */

typedef struct
{
    float err;
    float err_last;
    float expect;
    float feedback;
    float kp;
    float ki;
    float kd;
    float integral;
    float integral_max;
    float out;
    float out_max;
} Pid;

typedef struct
{
    Pid pitAngle;
    Pid rolAngle;
    Pid yawAngle;
    Pid pitGyro;
    Pid rolGyro;
    Pid yawGyro;
    Pid acc_high;
    Pid vel_high;
    Pid pos_high;
    Pid acc_fix_x;
    Pid vel_fix_x;
    Pid pos_fix_x;
    Pid acc_fix_y;
    Pid vel_fix_y;
    Pid pos_fix_y;
} AllPid;

float PidController(Pid *controller);
void AllPidInit(void);
void ClearIntegral(Pid *controller);

#endif
