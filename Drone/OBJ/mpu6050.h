#ifndef __MPU6050_H
#define __MPU6050_H

#include "iic.h"

typedef struct
{
    float x;
    float y;
    float z;
} FloatXYZ;

typedef struct
{
    FloatXYZ deg_s;
    FloatXYZ rad_s;
    FloatXYZ acc_g;
} Mpu6050_Data;

#define MPU6050_SELF_TESTX_REG      0X0D
#define MPU6050_SELF_TESTY_REG      0X0E
#define MPU6050_SELF_TESTZ_REG      0X0F
#define MPU6050_SELF_TESTA_REG      0X10
#define MPU6050_SAMPLE_RATE_REG     0X19
#define MPU6050_CFG_REG             0X1A
#define MPU6050_GYRO_CFG_REG        0X1B
#define MPU6050_ACCEL_CFG_REG       0X1C
#define MPU6050_MOTION_DET_REG      0X1F
#define MPU6050_FIFO_EN_REG         0X23
#define MPU6050_I2CMST_CTRL_REG     0X24
#define MPU6050_I2CSLV0_ADDR_REG    0X25
#define MPU6050_I2CSLV0_REG         0X26
#define MPU6050_I2CSLV0_CTRL_REG    0X27
#define MPU6050_I2CSLV1_ADDR_REG    0X28
#define MPU6050_I2CSLV1_REG         0X29
#define MPU6050_I2CSLV1_CTRL_REG    0X2A
#define MPU6050_I2CSLV2_ADDR_REG    0X2B
#define MPU6050_I2CSLV2_REG         0X2C
#define MPU6050_I2CSLV2_CTRL_REG    0X2D
#define MPU6050_I2CSLV3_ADDR_REG    0X2E
#define MPU6050_I2CSLV3_REG         0X2F
#define MPU6050_I2CSLV3_CTRL_REG    0X30
#define MPU6050_I2CSLV4_ADDR_REG    0X31
#define MPU6050_I2CSLV4_REG         0X32
#define MPU6050_I2CSLV4_DO_REG      0X33
#define MPU6050_I2CSLV4_CTRL_REG    0X34
#define MPU6050_I2CSLV4_DI_REG      0X35
#define MPU6050_I2CMST_STA_REG      0X36
#define MPU6050_INTBP_CFG_REG       0X37
#define MPU6050_INT_EN_REG          0X38
#define MPU6050_INT_STA_REG         0X3A
#define MPU6050_ACCEL_XOUTH_REG     0X3B
#define MPU6050_ACCEL_XOUTL_REG     0X3C
#define MPU6050_ACCEL_YOUTH_REG     0X3D
#define MPU6050_ACCEL_YOUTL_REG     0X3E
#define MPU6050_ACCEL_ZOUTH_REG     0X3F
#define MPU6050_ACCEL_ZOUTL_REG     0X40
#define MPU6050_TEMP_OUTH_REG       0X41
#define MPU6050_TEMP_OUTL_REG       0X42
#define MPU6050_GYRO_XOUTH_REG      0X43
#define MPU6050_GYRO_XOUTL_REG      0X44
#define MPU6050_GYRO_YOUTH_REG      0X45
#define MPU6050_GYRO_YOUTL_REG      0X46
#define MPU6050_GYRO_ZOUTH_REG      0X47
#define MPU6050_GYRO_ZOUTL_REG      0X48
#define MPU6050_I2CSLV0_DO_REG      0X63
#define MPU6050_I2CSLV1_DO_REG      0X64
#define MPU6050_I2CSLV2_DO_REG      0X65
#define MPU6050_I2CSLV3_DO_REG      0X66
#define MPU6050_I2CMST_DELAY_REG    0X67
#define MPU6050_SIGPATH_RST_REG     0X68
#define MPU6050_MDETECT_CTRL_REG    0X69
#define MPU6050_USER_CTRL_REG       0X6A
#define MPU6050_PWR_MGMT1_REG       0X6B
#define MPU6050_PWR_MGMT2_REG       0X6C
#define MPU6050_FIFO_CNTH_REG       0X72
#define MPU6050_FIFO_CNTL_REG       0X73
#define MPU6050_FIFO_RW_REG         0X74
#define MPU6050_DEVICE_ID_REG       0X75

#define MPU6050_ADDR                0X68

u8 MPU6050_Init(void);
u8 MPU6050_Write_Len(u8 addr, u8 reg, u8 len, u8 *buf);
u8 MPU6050_Read_Len(u8 addr, u8 reg, u8 len, u8 *buf);
u8 MPU6050_Write_Byte(u8 reg, u8 data);
u8 MPU6050_Read_Byte(u8 reg);
u8 MPU6050_Set_Gyro_Fsr(u8 fsr);
u8 MPU6050_Set_Accel_Fsr(u8 fsr);
u8 MPU6050_Set_LPF(u16 lpf);
u8 MPU6050_Set_Rate(u16 rate);
u8 MPU6050_Set_Fifo(u8 sens);
short MPU6050_Get_Temperature(void);
u8 MPU6050_Get_Gyroscope(short *gx, short *gy, short *gz);
u8 MPU6050_Get_Accelerometer(short *ax, short *ay, short *az);

#endif
