/*
 * ???????????????????? DMP ??????????????
 */
#include "imu.h"
#include "imath.h"
#include "math.h"
#include "mpu6050.h"
#include "timer.h"
#include "nav.h"
#include "inv_mpu.h"
#include "led.h"
#include "systick.h"


#define GYRO_RAW_TO_DEG_S       0.06103515625f   //速率 = ADC值/灵敏度；+-2000°/s:16.384LSB/°/s
Attitude att = {0};
_Time_test att_time;
extern PlaneData plane;
// 通过 MPU6050 的 DMP 获取姿态角。
// 姿态解算主要由 DMP 完成，这里只负责读取 roll/pitch/yaw。
void IMU(float *roll,float *pitch,float *yaw)
{
//	u8 res;
    time_check(&att_time);	 // 记录两次姿态更新之间的时间间隔
	mpu_dmp_get_data(roll,pitch,yaw);		// 飞机静止且 DMP 正常时可稳定获取姿态角
//	if(res == 0){
////		plane.dmp = DMP_NORMAL;
////		RGB_LedBlink(WHITE);
////		RGB_LedBlink(CYAN);
//	} else {
////		plane.dmp = DMP_NOT;
////		RGB_LedBlink(WHITE);
//	}
}

// 将陀螺仪原始 ADC 数据换算成角速度（度/秒）。
// 当前量程按 +-2000 deg/s 计算，对应 16.384 LSB/(deg/s)。
void GyroDataTransformDeg(float *gyroX,float *gyroY,float *gyroZ)
{
	short gX,gY,gZ;
	MPU6050_Get_Gyroscope(&gX,&gY,&gZ);
	
	*gyroX = (float)(gX * GYRO_RAW_TO_DEG_S);
	*gyroY = (float)(gY * GYRO_RAW_TO_DEG_S);
	*gyroZ = (float)(gZ * GYRO_RAW_TO_DEG_S);
}
