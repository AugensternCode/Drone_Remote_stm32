#ifndef _nrf24l01_h_
#define _nrf24l01_h_

#include "stm32f10x.h"

/*
 * nRF24L01 通过 SPI 与 MCU 通信。
 *
 * 要特别注意：
 * 1. 下面前一组宏大多是“SPI 命令字”，不是寄存器地址。
 * 2. 真正的寄存器地址是 CONFIG、STATUS、RF_CH 这一组。
 * 3. 访问寄存器时，通常需要把“命令字”和“寄存器地址”组合起来使用。
 *
 * 例如：
 * - 读取 STATUS 寄存器：NRF_READ_REG  + STATUS = 0x00 + 0x07 = 0x07
 * - 写入 STATUS 寄存器：NRF_WRITE_REG + STATUS = 0x20 + 0x07 = 0x27
 * - 写入 CONFIG 寄存器：NRF_WRITE_REG + CONFIG = 0x20 + 0x00 = 0x20
 */

/*============================== SPI 命令字 ==============================*/

/* 读寄存器命令基值。
 * 实际发送时一般写成：NRF_READ_REG + reg
 * 其中 reg 是寄存器地址。
 */
#define NRF_READ_REG    0x00

/* 写寄存器命令基值。
 * 这不是寄存器地址，而是 W_REGISTER 命令的基值。
 * 实际发送时一般写成：NRF_WRITE_REG + reg
 */
#define NRF_WRITE_REG   0x20

/* 读取 RX FIFO 中当前收到的一包有效载荷数据。 */
#define RD_RX_PLOAD     0x61

/* 向 TX FIFO 写入一包待发送的数据。 */
#define WR_TX_PLOAD     0xA0

/* 清空发送 FIFO。常在发送失败或准备重新发送前使用。 */
#define FLUSH_TX        0xE1

/* 清空接收 FIFO。常在丢弃无效包或异常恢复时使用。 */
#define FLUSH_RX        0xE2

/* 重用上一包 TX payload。
 * 在某些特殊发送流程里会用到，本工程里通常不常用。
 */
#define REUSE_TX_PL     0xE3

/* 空操作命令。
 * 发送 NOP 时不会执行实际功能，常用于“占位时钟”并顺带读回 STATUS。
 */
#define NOP             0xFF

/* 给 ACK 应答包写入有效载荷。
 * 注意这也是命令基值，实际使用时通常还要加 pipe 编号：
 * W_ACK_PLOAD + pipe
 */
#define W_ACK_PLOAD     0xA8

/*============================= 寄存器地址 =============================*/

/* 配置寄存器。
 * 用于设置收发模式、CRC 开关、CRC 位数、电源状态、中断屏蔽等。
 */
#define CONFIG          0x00

/* 自动应答使能寄存器。
 * 每一位对应一个数据管道，置 1 表示该管道启用自动应答。
 */
#define EN_AA           0x01

/* 接收地址使能寄存器。
 * 每一位对应一个 RX pipe，置 1 表示打开该接收通道。
 */
#define EN_RXADDR       0x02

/* 地址宽度设置寄存器。
 * 常见配置值 0x03 表示地址宽度为 5 字节。
 */
#define SETUP_AW        0x03

/* 自动重发设置寄存器。
 * 包含自动重发延时和最大重发次数两个配置项。
 */
#define SETUP_RETR      0x04

/* 射频频道寄存器。
 * 用来配置工作信道，例如 2.4GHz 频段中的某个频道。
 */
#define RF_CH           0x05

/* 射频参数寄存器。
 * 用于配置数据速率、发射功率、LNA 等。
 */
#define RF_SETUP        0x06

/* 状态寄存器。
 * 记录发送完成、接收完成、最大重发、当前 RX pipe 等状态信息。
 */
#define STATUS          0x07

/* 发送观测寄存器。
 * 可查看丢包统计、自动重发次数等信息，调试链路时很有用。
 */
#define OBSERVE_TX      0x08

/* 载波检测寄存器。
 * 用于判断当前信道上是否检测到载波。
 */
#define CD              0x09

/* 数据管道 0 的接收地址寄存器。 */
#define RX_ADDR_P0      0x0A

/* 数据管道 1 的接收地址寄存器。 */
#define RX_ADDR_P1      0x0B

/* 数据管道 2 的接收地址寄存器。
 * 注意 pipe2~pipe5 的高 4 字节通常继承 pipe1，只单独配置最低字节。
 */
#define RX_ADDR_P2      0x0C

/* 数据管道 3 的接收地址寄存器。 */
#define RX_ADDR_P3      0x0D

/* 数据管道 4 的接收地址寄存器。 */
#define RX_ADDR_P4      0x0E

/* 数据管道 5 的接收地址寄存器。 */
#define RX_ADDR_P5      0x0F

/* 发送地址寄存器。 */
#define TX_ADDR         0x10

/* 数据管道 0 固定负载长度寄存器。 */
#define RX_PW_P0        0x11

/* 数据管道 1 固定负载长度寄存器。 */
#define RX_PW_P1        0x12

/* 数据管道 2 固定负载长度寄存器。 */
#define RX_PW_P2        0x13

/* 数据管道 3 固定负载长度寄存器。 */
#define RX_PW_P3        0x14

/* 数据管道 4 固定负载长度寄存器。 */
#define RX_PW_P4        0x15

/* 数据管道 5 固定负载长度寄存器。 */
#define RX_PW_P5        0x16

/* FIFO 状态寄存器。
 * 可用于判断 TX/RX FIFO 是否为空、是否满等。
 */
#define NRF_FIFO_STATUS 0x17

/* 动态负载长度使能寄存器。
 * 某一位为 1，表示对应 pipe 启用动态 payload 长度。
 */
#define DYNPD           0x1C

/* 特性寄存器。
 * 常用于开启动态负载长度、ACK 负载、动态 ACK 等功能。
 */
#define FEATURE         0x1D

/*=========================== STATUS 标志位 ===========================*/

/* 达到最大自动重发次数。
 * 发送端在自动重发达到上限后会置位此标志。
 */
#define MAX_TX          0x10

/* 发送成功标志（TX_DS）。
 * 一包数据成功发送并收到 ACK 后会置位。
 */
#define TX_OK           0x20

/* 接收成功标志（RX_DR）。
 * RX FIFO 中收到新数据后会置位。
 */
#define RX_OK           0x40

/*============================= GPIO 控制 =============================*/

/* CE 拉低。
 * 常用于待机、切换模式、写配置寄存器前的准备。
 */
#define NRF_CE_L        GPIO_ResetBits(GPIOB, GPIO_Pin_1)

/* CE 拉高。
 * 在不同模式下，CE 拉高会触发接收或发射动作。
 */
#define NRF_CE_H        GPIO_SetBits(GPIOB, GPIO_Pin_1)

/* CSN 拉低，表示开始一次 SPI 访问。 */
#define SPI_CSN_L       GPIO_ResetBits(GPIOB, GPIO_Pin_0)

/* CSN 拉高，表示结束一次 SPI 访问。 */
#define SPI_CSN_H       GPIO_SetBits(GPIOB, GPIO_Pin_0)

/* 读取 IRQ 引脚电平。
 * nRF24L01 的中断通常为低电平有效。
 */
#define NRF_IRQ         GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4)

/*========================== 地址/数据长度配置 ==========================*/

/* TX 地址宽度，单位：字节。 */
#define TX_ADR_WIDTH    5

/* RX 地址宽度，单位：字节。 */
#define RX_ADR_WIDTH    5

/* 一次发送的数据长度，单位：字节。
 * 如果工程启用了固定长度 payload，这个值需要和发送端、接收端一致。
 */
#define TX_PLOAD_WIDTH  11

/* 一次接收的数据长度，单位：字节。 */
#define RX_PLOAD_WIDTH  11

/*============================= 底层 SPI 接口 =============================*/

/* 从指定寄存器/命令位置连续写入多个字节。 */
u8 SPI_Write_Buf(u8 reg, u8 *pBuf, u8 u8s);

/* 从指定寄存器/命令位置连续读取多个字节。 */
u8 SPI_Read_Buf(u8 reg, u8 *pBuf, u8 u8s);

/* 读取单字节寄存器内容。 */
u8 SPI_Read_Reg(u8 reg);

/* 写入单字节寄存器内容。 */
u8 SPI_Write_Reg(u8 reg, u8 value);

/*============================= 功能接口 =============================*/

/* nRF24L01 初始化。 */
void NRF24L01Init(void);

/* 切换到接收模式。 */
void NRF24L01ReceiveMode(void);

/* 切换到发送模式。 */
void NRF24L01_TX_Mode(void);

/* 检查 nRF24L01 是否在线。
 * 常见做法是写入 TX_ADDR 后再读回校验。
 */
u8 NRF24L01_Check(void);

/* 发送一包数据。 */
u8 NRF24L01_TxPacket(u8 *txbuf);

/* 接收一包数据。 */
u8 NRF24L01_RxPacket(u8 *rxbuf);

#endif
