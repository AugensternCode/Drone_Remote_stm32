#ifndef __MPU6050_H
#define __MPU6050_H

#include "iic.h" // 包含 IIC 通信协议的头文件，用于底层数据传输

/**
 * @brief 结构体：表示一个包含 x、y、z 三个分量的浮点型向量
 * 通常用于表示加速度（单位：g）、角速度（单位：度/秒 或 弧度/秒）
 */
typedef struct
{
    float x;
    float y;
    float z;
} FloatXYZ;

/**
 * @brief 结构体：用于存储 MPU6050 传感器读取并转换后的核心数据
 * - deg_s: 角速度，单位是度每秒 (degree per second)
 * - rad_s: 角速度，单位是弧度每秒 (radians per second)
 * - acc_g: 加速度，单位是重力加速度 (g)
 */
typedef struct
{
    FloatXYZ deg_s;   // 陀螺仪数据（度/秒）
    FloatXYZ rad_s;   // 陀螺仪数据（弧度/秒）
    FloatXYZ acc_g;   // 加速度计数据（g）
} Mpu6050_Data;

/* --- 以下是 MPU6050 内部寄存器的地址定义 --- */

/* 自检寄存器 */
#define MPU6050_SELF_TESTX_REG      0X0D // X轴自检
#define MPU6050_SELF_TESTY_REG      0X0E // Y轴自检
#define MPU6050_SELF_TESTZ_REG      0X0F // Z轴自检
#define MPU6050_SELF_TESTA_REG      0X10 // 加速度计自检

/* 采样与配置寄存器 */
#define MPU6050_SAMPLE_RATE_REG     0X19 // 采样率分频器
#define MPU6050_CFG_REG             0X1A // 配置寄存器（数字低通滤波器 DLPF 等）

/* 量程设置寄存器 */
#define MPU6050_GYRO_CFG_REG        0X1B // 陀螺仪配置寄存器（量程选择）
#define MPU6050_ACCEL_CFG_REG       0X1C // 加速度计配置寄存器（量程选择）

/* 运动检测与中断 */
#define MPU6050_MOTION_DET_REG      0X1F // 运动检测阈值
#define MPU6050_FIFO_EN_REG         0X23 // FIFO 使能寄存器

/* I2C 主模式控制（用于连接外部传感器，如磁力计） */
#define MPU6050_I2CMST_CTRL_REG     0X24 // I2C 主模式控制
#define MPU6050_I2CSLV0_ADDR_REG    0X25 // 从机0地址
#define MPU6050_I2CSLV0_REG         0X26 // 从机0寄存器地址
#define MPU6050_I2CSLV0_CTRL_REG    0X27 // 从机0控制
#define MPU6050_I2CSLV1_ADDR_REG    0X28 // 从机1地址
#define MPU6050_I2CSLV1_REG         0X29 // 从机1寄存器地址
#define MPU6050_I2CSLV1_CTRL_REG    0X2A // 从机1控制
#define MPU6050_I2CSLV2_ADDR_REG    0X2B // 从机2地址
#define MPU6050_I2CSLV2_REG         0X2C // 从机2寄存器地址
#define MPU6050_I2CSLV2_CTRL_REG    0X2D // 从机2控制
#define MPU6050_I2CSLV3_ADDR_REG    0X2E // 从机3地址
#define MPU6050_I2CSLV3_REG         0X2F // 从机3寄存器地址
#define MPU6050_I2CSLV3_CTRL_REG    0X30 // 从机3控制
#define MPU6050_I2CSLV4_ADDR_REG    0X31 // 从机4地址
#define MPU6050_I2CSLV4_REG         0X32 // 从机4寄存器地址
#define MPU6050_I2CSLV4_DO_REG      0X33 // 从机4写数据
#define MPU6050_I2CSLV4_CTRL_REG    0X34 // 从机4控制
#define MPU6050_I2CSLV4_DI_REG      0X35 // 从机4读数据

/* I2C 主模式状态与中断 */
#define MPU6050_I2CMST_STA_REG      0X36 // I2C 主模式状态
#define MPU6050_INTBP_CFG_REG       0X37 // 中断引脚配置
#define MPU6050_INT_EN_REG          0X38 // 中断使能
#define MPU6050_INT_STA_REG         0X3A // 中断状态

/* 加速度计数据寄存器（高8位和低8位分开存储） */
#define MPU6050_ACCEL_XOUTH_REG     0X3B // X轴加速度高字节
#define MPU6050_ACCEL_XOUTL_REG     0X3C // X轴加速度低字节
#define MPU6050_ACCEL_YOUTH_REG     0X3D // Y轴加速度高字节
#define MPU6050_ACCEL_YOUTL_REG     0X3E // Y轴加速度低字节
#define MPU6050_ACCEL_ZOUTH_REG     0X3F // Z轴加速度高字节
#define MPU6050_ACCEL_ZOUTL_REG     0X40 // Z轴加速度低字节

/* 温度传感器数据寄存器 */
#define MPU6050_TEMP_OUTH_REG       0X41 // 温度高字节
#define MPU6050_TEMP_OUTL_REG       0X42 // 温度低字节

/* 陀螺仪数据寄存器 */
#define MPU6050_GYRO_XOUTH_REG      0X43 // X轴角速度高字节
#define MPU6050_GYRO_XOUTL_REG      0X44 // X轴角速度低字节
#define MPU6050_GYRO_YOUTH_REG      0X45 // Y轴角速度高字节
#define MPU6050_GYRO_YOUTL_REG      0X46 // Y轴角速度低字节
#define MPU6050_GYRO_ZOUTH_REG      0X47 // Z轴角速度高字节
#define MPU6050_GYRO_ZOUTL_REG      0X48 // Z轴角速度低字节

/* I2C 从机数据输出寄存器 */
#define MPU6050_I2CSLV0_DO_REG      0X63 // 从机0数据
#define MPU6050_I2CSLV1_DO_REG      0X64 // 从机1数据
#define MPU6050_I2CSLV2_DO_REG      0X65 // 从机2数据
#define MPU6050_I2CSLV3_DO_REG      0X66 // 从机3数据
#define MPU6050_I2CMST_DELAY_REG    0X67 // I2C 主模式延时

/* 其他控制寄存器 */
#define MPU6050_SIGPATH_RST_REG     0X68 // 信号路径复位
#define MPU6050_MDETECT_CTRL_REG    0X69 // 运动检测控制
#define MPU6050_USER_CTRL_REG       0X6A // 用户控制（FIFO、I2C 主模式等）
#define MPU6050_PWR_MGMT1_REG       0X6B // 电源管理1（时钟源、休眠、复位）
#define MPU6050_PWR_MGMT2_REG       0X6C // 电源管理2（各轴待机控制）

/* FIFO 计数与读写寄存器 */
#define MPU6050_FIFO_CNTH_REG       0X72 // FIFO 计数高字节
#define MPU6050_FIFO_CNTL_REG       0X73 // FIFO 计数低字节
#define MPU6050_FIFO_RW_REG         0X74 // FIFO 读写寄存器

/* 设备 ID 寄存器 */
#define MPU6050_DEVICE_ID_REG       0X75 // 设备 ID（应读回 0x68）

/**
 * @brief MPU6050 的 I2C 设备地址
 * 当 AD0 引脚接低电平时地址为 0x68，接高电平时为 0x69
 */
#define MPU6050_ADDR                0X68

/* --- 外部函数声明（API 接口）--- */

u8 MPU6050_Init(void); // 初始化 MPU6050，配置量程、滤波器等，成功返回 1

u8 MPU6050_Write_Len(u8 addr, u8 reg, u8 len, u8 *buf); // 向指定寄存器连续写入多个字节
u8 MPU6050_Read_Len(u8 addr, u8 reg, u8 len, u8 *buf);  // 从指定寄存器连续读取多个字节

u8 MPU6050_Write_Byte(u8 reg, u8 data); // 向指定寄存器写入一个字节
u8 MPU6050_Read_Byte(u8 reg);           // 从指定寄存器读取一个字节

u8 MPU6050_Set_Gyro_Fsr(u8 fsr);   // 设置陀螺仪量程（例如 ±250/500/1000/2000 °/s）
u8 MPU6050_Set_Accel_Fsr(u8 fsr);  // 设置加速度计量程（例如 ±2/4/8/16 g）
u8 MPU6050_Set_LPF(u16 lpf);       // 设置数字低通滤波器（DLPF）截止频率
u8 MPU6050_Set_Rate(u16 rate);     // 设置采样率（Hz）

u8 MPU6050_Set_Fifo(u8 sens);      // 设置 FIFO 使能（哪些数据存入 FIFO）

short MPU6050_Get_Temperature(void); // 获取温度原始值（需转换公式得到摄氏度）
u8 MPU6050_Get_Gyroscope(short *gx, short *gy, short *gz);   // 获取陀螺仪原始值（ADC 码值）
u8 MPU6050_Get_Accelerometer(short *ax, short *ay, short *az); // 获取加速度计原始值（ADC 码值）

#endif /* __MPU6050_H */