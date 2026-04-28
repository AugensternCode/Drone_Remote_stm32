/*
 * ????????????????????????????????
 */
#include "nav.h"
#include "mpu6050.h"
#include "imu.h"
#include "math.h"


_NAV_DATA nav = {0};


//void get_nav_acc(SI_F_XYZ acc_lpf)
//{
//    //载体加速度转到导航系下的三轴加速度
//    nav.acc[0] =    Mat.DCM[0][0] * acc_lpf.x
//                  + Mat.DCM[0][1] * acc_lpf.y
//                  + Mat.DCM[0][2] * acc_lpf.z;
//    
//    nav.acc[1] =    Mat.DCM[1][0] * acc_lpf.x
//                  + Mat.DCM[1][1] * acc_lpf.y
//                  + Mat.DCM[1][2] * acc_lpf.z;
//    
//    nav.acc[2] =    Mat.DCM[2][0] * acc_lpf.x
//                  + Mat.DCM[2][1] * acc_lpf.y
//                  + Mat.DCM[2][2] * acc_lpf.z;      

//    nav.acc[0] *= acc_to_1g;
//    nav.acc[0] *= 100;                    //cm/s^2
//    
//    nav.acc[1] *= acc_to_1g;
//    nav.acc[1] *= 100;                    //cm/s^2   

//    nav.acc[2] *= acc_to_1g;
//    nav.acc[2] -= gravity_mss;
//    nav.acc[2] *= 100;                    //cm/s^2
//     
//    nav.acc_lenth = sqrt( nav.acc[2] * nav.acc[2]
//                        + nav.acc[0] * nav.acc[0]
//                        + nav.acc[1] * nav.acc[1]);    

////    integral_vel();
//}


//加速度积分得速度
void integral_vel(void)
{
    nav.integral_vel[0] += nav.acc[0]*0.005f;
    nav.integral_vel[1] += nav.acc[1]*0.005f;
    nav.integral_vel[2] += nav.acc[2]*0.005f;
}





