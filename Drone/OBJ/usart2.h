#ifndef _usart2_h_
#define _usart2_h_

#include "stm32f10x.h"
#include "stdio.h"

/*
 * USART2 ???????????
 *
 * ???????????????PID ?????
 */

#define BYTE0(dwTemp)  (*((char *)(&dwTemp)))
#define BYTE1(dwTemp)  (*((char *)(&dwTemp) + 1))
#define BYTE2(dwTemp)  (*((char *)(&dwTemp) + 2))
#define BYTE3(dwTemp)  (*((char *)(&dwTemp) + 3))

void Usart2Init(u32 bound);
void Usart2Task(void);
void ANOSendStatus(void);
void ANO_DT_Send_RCData(u16 throttle, u16 yaw, u16 rol, u16 pit,
                        u16 aux1, u16 aux2, u16 aux3, u16 aux4, u16 aux5, u16 aux6);

#endif
