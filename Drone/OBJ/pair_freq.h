#ifndef _pair_freq_h_
#define _pair_freq_h_

#include "stm32f10x.h"

/*
 * 无线对频模块。
 *
 * 飞控上电后，先使用默认地址和默认信道监听遥控器发来的配对包。
 * 收到合法配对包后，再把 nRF24L01 切换到新的通信地址和新信道。
 */

typedef struct
{
    uint8_t addr[5];        /* nRF24L01 的 5 字节通信地址 */
    uint8_t freq_channel;   /* nRF24L01 的工作信道 */
} PairInfo;

extern PairInfo pair;

/* 等待配对包并更新无线参数。 */
void wait_pairing(void);

#endif
