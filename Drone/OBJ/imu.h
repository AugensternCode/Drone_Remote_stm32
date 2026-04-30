#ifndef _imu_h_
#define _imu_h_

/*
 * IMU 姿态数据模块。
 *
 * 当前姿态角主要由 MPU6050 的 InvenSense DMP 解算得到，
 * 本模块负责保存姿态角并提供姿态读取、矩阵更新等接口。
 */

typedef struct
{
    float pit;
    float rol;
    float yaw;
} Attitude;

extern Attitude att;
extern float sin_pit, sin_rol, sin_yaw;
extern float cos_pit, cos_rol, cos_yaw;

/* 使用原始陀螺仪/加速度计数据更新 IMU 姿态。 */
void IMUUpdate(float gx, float gy, float gz, float ax, float ay, float az);

/* 更新姿态旋转矩阵及其转置矩阵。 */
void rotation_matrix(void);
void rotation_matrix_T(void);
void Matrix_ready(void);

/* 从 MPU6050 DMP 读取 roll/pitch/yaw 姿态角。 */
void IMU(float *roll, float *pitch, float *yaw);

/* 将 MPU6050 陀螺仪原始数据转换为角速度，单位为 deg/s。 */
void GyroDataTransformDeg(float *gyroX, float *gyroY, float *gyroZ);

#endif
