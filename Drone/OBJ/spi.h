#ifndef _spi_h_
#define _spi_h_

#include "stm32f10x.h"

/*
 * SPI1 底层驱动。
 *
 * 当前主要服务于 nRF24L01，
 * 提供 SPI 外设初始化以及单字节全双工收发接口。
 */

/* 初始化 SPI1 和相关 GPIO。 */
void SpiInit(void);

/* 发送一个字节，同时接收一个字节。 */
u8 Spi_RW_Byte(u8 TxData);

#endif
