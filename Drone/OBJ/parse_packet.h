#ifndef _parse_packet_h_
#define _parse_packet_h_

#include "nrf24l01.h"
#include "stm32f10x.h"

/*
 * 无线数据包解析模块。
 *
 * plane 结构体集中保存遥控输入、链路状态、电池状态、
 * 配对状态以及 DMP 状态等飞控运行时信息。
 */

typedef enum{
    LOCK = 0,
    UNLOCK,
} Lock;      /* 锁定状态 */

typedef enum{
    SIGNAL_LOST = 0,
    SIGNAL_NORMAL,
} Signal;    /* 无线信号状态 */

typedef enum{
    POWER_NORMAL = 0,
    POWER_LOWER,
} Power;     /* 电量状态 */

typedef enum{
    PAIR_NOT = 0,
    PAIR_NORMAL,
} Pair;      /* 配对状态 */

typedef enum{
    DMP_NOT = 0,
    DMP_NORMAL,
} Dmp;       /* DMP 工作状态 */

typedef struct
{
    uint16_t throttle;      /* 油门 */
    float pit;              /* 俯仰控制量 */
    float rol;              /* 横滚控制量 */
    float yaw;              /* 偏航控制量 */
    uint8_t key_l;          /* 左功能键 */
    uint8_t key_r;          /* 右功能键 */
    Lock lock;              /* 上锁状态 */
    Signal signal;          /* 无线链路状态 */
    Power power;            /* 电量状态 */
    Pair pair;              /* 配对状态 */
    Dmp dmp;                /* DMP 获取状态 */
    uint16_t unlockCount;   /* 解锁动作持续计数 */
    uint16_t lockCount;     /* 上锁动作持续计数 */
    uint8_t signalLostCount;/* 丢包累计计数 */
    float voltage;          /* 电池电压 */
} PlaneData;

extern u8 Rx_packet[RX_PLOAD_WIDTH];

/* 解析 nRF24L01 收到的遥控控制包。 */
void nrf_parse_packet(void);

/* 预留的按键解析接口。 */
void parse_key_info(void);

/* 通过 ACK 负载回传飞控状态。 */
void NrfACKPacket(void);

#endif
