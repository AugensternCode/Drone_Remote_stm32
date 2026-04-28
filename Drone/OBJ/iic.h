#ifndef _iic_H
#define _iic_H
#include "system.h"
/*
 * 软件 I2C 驱动
 *
 * 使用 GPIO 模拟 I2C 时序
 * 适用于 MPU6050 和 OLED 等设备
 */
/* I2C SCL 引脚定义 */
#define IIC_SCL_PORT      GPIOB
#define IIC_SCL_PIN       (GPIO_Pin_6)
#define IIC_SCL_PORT_RCC  RCC_APB2Periph_GPIOB
/* I2C SDA 引脚定义 */
#define IIC_SDA_PORT      GPIOB
#define IIC_SDA_PIN       (GPIO_Pin_5)
#define IIC_SDA_PORT_RCC  RCC_APB2Periph_GPIOB

/* 设置高低电平 SCL / SDA 宏 */
#define IIC_SCL    PBout(6)
#define IIC_SDA    PBout(5)
#define READ_SDA   PBin(5)
/* 初始化 I2C 的 GPIO */
void IIC_Init(void);
/* 产生起始信号 */
void IIC_Start(void);
/* 产生停止信号 */
void IIC_Stop(void);
/* 发送一个字节 */
void IIC_Send_Byte(u8 txd);
/* 读取一个字节，ack=1 时发送 ACK，ack=0 时发送 NACK */
u8 IIC_Read_Byte(u8 ack);
/* 等待应答信号，返回 0 表示收到 ACK，1 表示未收到 ACK */
u8 IIC_Wait_Ack(void);
/* 产生 ACK 应答 */
void IIC_Ack(void);
/* 产生 NACK 非应答 */
void IIC_NAck(void);
#endif
