/*
 * ?????PID ?????? PID ?????
 */
#include "pid.h"
#include "flash.h"

AllPid allPid;
uint32_t pidResetFlag = 0;

//存储pid控制器参数
const float  pidIintData[15][5] =
{
    //0.kp 1.ki 2.kd 3.积分限幅  4.pid输出限幅值  
    //姿态外环参数  
    {2.5,  0,   0,  300, 	800},          //俯仰角度值
    {2.5,  0,   0,  300,	800},          //横滚角度值
    {2.0,  0,   0,  300, 	800},          //偏航角度值
    
    //姿态内环参数
    {0.60,   0.0010,  0.010,  300, 	800},    //俯仰角速度值
    {0.60,   0.0010,  0.010,  300, 	800},    //横贯角速度值
    {1.35,   0.0020,  0.155,  300, 	800},    //偏航角速度值
    
		//竖直定高参数
		{0,		0,	 0,	 0,		0},		//acc_high
    {0,		0,	 0,	 0,		0},		//vel_high
    {0,		0,	 0,	 0,		0},		//pos_high
		
		//x方向参数 
		{0,		0,	 0,	 0,		0},		//acc_high
    {0,		0,	 0,	 0,		0},		//vel_high
    {0,		0,	 0,	 0,		0},		//pos_x
		
		//y方向参数
		{0,		0,	 0,	 0,		0},		//acc_y
    {0,		0,	 0,	 0,		0},		//vel_y
    {0,		0,	 0,	 0,		0},		//pos_y
          
};

//pid参数初始化配置
void PidInit(Pid *controller,uint8_t label)
{
    controller->kp  = pidIintData[label][0];
    controller->ki  = pidIintData[label][1];
    controller->kd  = pidIintData[label][2];
    controller->integral_max = pidIintData[label][3];
    controller->out_max = pidIintData[label][4];      
}
//pid参数初始化
void AllPidInit(void)
{
	uint32_t res;
	
	PidInit(&allPid.pitAngle,0);
	PidInit(&allPid.rolAngle,1);
	PidInit(&allPid.yawAngle,2);
	
	PidInit(&allPid.pitGyro,3);
	PidInit(&allPid.rolGyro,4);
	PidInit(&allPid.yawGyro,5);

	PidInit(&allPid.acc_high,6);
	PidInit(&allPid.vel_high,7);
	PidInit(&allPid.pos_high,8);
	
	PidInit(&allPid.acc_fix_x,9);
	PidInit(&allPid.vel_fix_x,10);
	PidInit(&allPid.pos_fix_x,11);    
	
	PidInit(&allPid.acc_fix_y,12);
	PidInit(&allPid.vel_fix_y,13);
	PidInit(&allPid.pos_fix_y,14);
	
	res = *(uint32_t *)(PID_WRITE_ADDRESS);
	if(res == 10 && pidResetFlag == 0){		//判断帧头及复位标志位
		PidDataReadFromFlash(PID_WRITE_ADDRESS,&allPid);		//pid数据从flash读出
	}else{
		PidDataWriteToFlash(PID_WRITE_ADDRESS,&allPid);		//pid数据写入flash
	}
	
} 

//pid控制器
float PidController(Pid *controller)
{
    controller->err_last = controller->err;		//保存上次偏差
    controller->err = controller->expect - controller->feedback;		//计算误差
    controller->integral += controller->ki * controller->err;		//误差积分  

    //积分限幅
    if(controller->integral >  controller->integral_max){
			controller->integral =  controller->integral_max;
		}
    if(controller->integral < -controller->integral_max){
			controller->integral = -controller->integral_max;
		}
 
    //pid运算
    controller->out =  controller->kp * controller->err
                     + controller->integral
                     + controller->kd * (controller->err - controller->err_last);
    //输出限幅
    if(controller->out >  controller->out_max)   controller->out =  controller->out_max;
    if(controller->out < -controller->out_max)   controller->out = -controller->out_max;

    return  controller->out;
}

//清除积分
void ClearIntegral(Pid *controller)
{
    controller->integral = 0.0f;
}
