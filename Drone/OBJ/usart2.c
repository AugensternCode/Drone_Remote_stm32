/*
 * ?????USART2 ??????????????????PID ????????
 */
#include "usart2.h"
#include "imu.h"
#include "parse_packet.h"
#include "pid.h"
#include "systick.h"
#include "led.h"
#include "flash.h"

extern uint32_t pidResetFlag;
extern AllPid allPid;
extern PlaneData plane;

// DMA 发送、协议组帧和协议解析都只在本文件内部使用。
static void Usart2DmaConfig(DMA_Channel_TypeDef* DMA_CHx,u32 peripheral_addr,u32 memory_addr,u16 data_length);
static u8 UsartDMASendData(const u8 *sendBuff,u16 len);
static void Usart2DmaPoll(void);
static void Usart2WaitForDmaIdle(void);
static void PidDataSend(u16 *sendBuff,u8 funcByte,u8 dataLen);
static void DateTransfer(void);
static void Usart2Send(const u8 *data,u8 len);
static void ANO_DT_Send_Check(u8 head, u8 check_sum);
static void ANODataReceiveAnalysis(u8 *dataBuffer,u8 num);
static void ANODTDataReceivePrepare(u8 data);

#define USART2_DMA_TX_BUF_LEN   64
#define ANO_RX_FRAME_BUF_LEN    50

static uint8_t usart2_dma_tx_buf[USART2_DMA_TX_BUF_LEN] = {0};
static volatile uint8_t usart2_dma_busy = 0;

static volatile u8 ano_pending_frame[ANO_RX_FRAME_BUF_LEN] = {0};
static volatile u8 ano_pending_len = 0;
static volatile u8 ano_frame_pending = 0;

// USART2 用于匿名上位机通信：状态遥测、PID 读写和在线恢复默认参数。
void Usart2Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};
	USART_InitTypeDef USART_InitStructure = {0};

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART2, &USART_InitStructure);

	USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
	USART_Cmd(USART2, ENABLE);
}

// 配置 USART2 TX 对应的 DMA 通道。
// 每次发送前都会重装长度，因此使用普通模式。
static void Usart2DmaConfig(DMA_Channel_TypeDef* DMA_CHx,u32 peripheral_addr,u32 memory_addr,u16 data_length)
{
	DMA_InitTypeDef DMA_InitStructure = {0};

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA_CHx);
	DMA_InitStructure.DMA_PeripheralBaseAddr = peripheral_addr;
	DMA_InitStructure.DMA_MemoryBaseAddr = memory_addr;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = data_length;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA_CHx, &DMA_InitStructure);
}

// 轮询 DMA 是否已经把本次发送缓冲区搬空。
static void Usart2DmaPoll(void)
{
	if(usart2_dma_busy && DMA_GetCurrDataCounter(DMA1_Channel7) == 0){
		DMA_Cmd(DMA1_Channel7, DISABLE);
		USART_DMACmd(USART2, USART_DMAReq_Tx, DISABLE);
		usart2_dma_busy = 0;
	}
}

// 阻塞等待 DMA 空闲，避免 DMA 发送和阻塞发送同时写 USART2->DR。
static void Usart2WaitForDmaIdle(void)
{
	while(usart2_dma_busy){
		Usart2DmaPoll();
	}
}

// 先复制到静态缓冲区，再启动 DMA，避免发送期间引用调用者的栈内存。
static u8 UsartDMASendData(const u8 *sendBuff,u16 len)
{
	u16 i;

	if(len == 0 || len > USART2_DMA_TX_BUF_LEN){
		return 0;
	}

	Usart2DmaPoll();
	if(usart2_dma_busy){
		return 0;
	}

	for(i = 0; i < len; i++){
		usart2_dma_tx_buf[i] = sendBuff[i];
	}

	DMA_Cmd(DMA1_Channel7, DISABLE);
	Usart2DmaConfig(DMA1_Channel7,(u32)&USART2->DR,(u32)usart2_dma_tx_buf,len);
	DMA_SetCurrDataCounter(DMA1_Channel7,len);
	USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA1_Channel7, ENABLE);
	usart2_dma_busy = 1;

	return 1;
}

// 主循环中的串口任务。
// 中断里只负责收字节和组帧，真正的解析和 Flash 写入放到这里执行。
void Usart2Task(void)
{
	u8 dataBuffer[ANO_RX_FRAME_BUF_LEN];
	u8 len = 0;
	u8 i;

	Usart2DmaPoll();

	if(!ano_frame_pending){
		return;
	}

	__disable_irq();
	len = ano_pending_len;
	if(len > ANO_RX_FRAME_BUF_LEN){
		len = ANO_RX_FRAME_BUF_LEN;
	}
	for(i = 0; i < len; i++){
		dataBuffer[i] = ano_pending_frame[i];
	}
	ano_frame_pending = 0;
	__enable_irq();

	ANODataReceiveAnalysis(dataBuffer,len);
}

// 向匿名上位机发送姿态角和锁定状态。
// 姿态角放大 100 倍后按 int16 发送，便于上位机按固定点格式解析。
void ANOSendStatus(void)
{
	u8 cnt = 0;
	vs16 temp;
	vs32 temp2;
	u8 sum = 0;
	u8 i;
	u8 dataToSend[50];

	dataToSend[cnt++] = 0xAA;
	dataToSend[cnt++] = 0xAA;
	dataToSend[cnt++] = 0x01;
	dataToSend[cnt++] = 0;

	temp = (int)(att.rol * 100);
	dataToSend[cnt++] = BYTE1(temp);
	dataToSend[cnt++] = BYTE0(temp);

	temp = (int)(att.pit * 100);
	dataToSend[cnt++] = BYTE1(temp);
	dataToSend[cnt++] = BYTE0(temp);

	temp = (int)(att.yaw * 100);
	dataToSend[cnt++] = BYTE1(temp);
	dataToSend[cnt++] = BYTE0(temp);

	temp2 = (int32_t)(0);
	dataToSend[cnt++] = BYTE3(temp2);
	dataToSend[cnt++] = BYTE2(temp2);
	dataToSend[cnt++] = BYTE1(temp2);
	dataToSend[cnt++] = BYTE0(temp2);

	dataToSend[cnt++] = 0x01;
	dataToSend[cnt++] = plane.lock;

	dataToSend[3] = cnt - 4;

	for(i = 0; i < cnt; i++){
		sum += dataToSend[i];
	}
	dataToSend[cnt++] = sum;

	UsartDMASendData(dataToSend,cnt);
}

// 发送遥控通道原始值，主要用于上位机观察输入量。
void ANO_DT_Send_RCData(u16 throttle,u16 yaw,u16 rol,u16 pit,u16 aux1,u16 aux2,u16 aux3,u16 aux4,u16 aux5,u16 aux6)
{
	u8 cnt = 0;
	u8 i = 0;
	u8 sum = 0;
	u8 dataToSend[50];

	dataToSend[cnt++] = 0xAA;
	dataToSend[cnt++] = 0xAA;
	dataToSend[cnt++] = 0x03;
	dataToSend[cnt++] = 0;

	dataToSend[cnt++] = BYTE1(throttle);
	dataToSend[cnt++] = BYTE0(throttle);
	dataToSend[cnt++] = BYTE1(yaw);
	dataToSend[cnt++] = BYTE0(yaw);
	dataToSend[cnt++] = BYTE1(rol);
	dataToSend[cnt++] = BYTE0(rol);
	dataToSend[cnt++] = BYTE1(pit);
	dataToSend[cnt++] = BYTE0(pit);
	dataToSend[cnt++] = BYTE1(aux1);
	dataToSend[cnt++] = BYTE0(aux1);
	dataToSend[cnt++] = BYTE1(aux2);
	dataToSend[cnt++] = BYTE0(aux2);
	dataToSend[cnt++] = BYTE1(aux3);
	dataToSend[cnt++] = BYTE0(aux3);
	dataToSend[cnt++] = BYTE1(aux4);
	dataToSend[cnt++] = BYTE0(aux4);
	dataToSend[cnt++] = BYTE1(aux5);
	dataToSend[cnt++] = BYTE0(aux5);
	dataToSend[cnt++] = BYTE1(aux6);
	dataToSend[cnt++] = BYTE0(aux6);

	dataToSend[3] = cnt - 4;

	for(i = 0; i < cnt; i++){
		sum += dataToSend[i];
	}
	dataToSend[cnt++] = sum;

	UsartDMASendData(dataToSend,cnt);
}

// 把一组 PID 参数按匿名上位机协议打包发送。
static void PidDataSend(u16 *sendBuff,u8 funcByte,u8 dataLen)
{
	u8 cnt = 0;
	vs16 temp;
	u8 sum = 0;
	u8 i;
	u8 dataToSend[50];

	dataToSend[cnt++] = 0xAA;
	dataToSend[cnt++] = 0xAA;
	dataToSend[cnt++] = funcByte;
	dataToSend[cnt++] = 0;

	for(i = 0; i < dataLen; i++){
		temp = (int)(sendBuff[i]);
		dataToSend[cnt++] = BYTE1(temp);
		dataToSend[cnt++] = BYTE0(temp);
	}

	dataToSend[3] = cnt - 4;

	for(i = 0; i < cnt; i++){
		sum += dataToSend[i];
	}
	dataToSend[cnt++] = sum;

	Usart2Send(dataToSend,cnt);
}

// 导出当前 PID 参数。
// 0x10 对应姿态角环，0x11 对应角速度环。
static void DateTransfer(void)
{
	u8 arrLen = 0;
	u16 arrTemp[10] = {0};

	arrTemp[arrLen++] = allPid.rolAngle.kp * 100;
	arrTemp[arrLen++] = allPid.rolAngle.ki * 100;
	arrTemp[arrLen++] = allPid.rolAngle.kd * 100;
	arrTemp[arrLen++] = allPid.pitAngle.kp * 100;
	arrTemp[arrLen++] = allPid.pitAngle.ki * 100;
	arrTemp[arrLen++] = allPid.pitAngle.kd * 100;
	arrTemp[arrLen++] = allPid.yawAngle.kp * 100;
	arrTemp[arrLen++] = allPid.yawAngle.ki * 100;
	arrTemp[arrLen++] = allPid.yawAngle.kd * 100;
	PidDataSend(arrTemp,0x10,arrLen);

	delay_ms(2);

	arrLen = 0;
	arrTemp[arrLen++] = allPid.rolGyro.kp * 100;
	arrTemp[arrLen++] = allPid.rolGyro.ki * 10000;
	arrTemp[arrLen++] = allPid.rolGyro.kd * 1000;
	arrTemp[arrLen++] = allPid.pitGyro.kp * 100;
	arrTemp[arrLen++] = allPid.pitGyro.ki * 10000;
	arrTemp[arrLen++] = allPid.pitGyro.kd * 1000;
	arrTemp[arrLen++] = allPid.yawGyro.kp * 100;
	arrTemp[arrLen++] = allPid.yawGyro.ki * 10000;
	arrTemp[arrLen++] = allPid.yawGyro.kd * 1000;
	PidDataSend(arrTemp,0x11,arrLen);
}

// 少量控制帧采用阻塞串口发送，逐字节等 TXE，末尾统一等一次 TC。
static void Usart2Send(const u8 *data,u8 len)
{
	u8 i;

	Usart2WaitForDmaIdle();
	for(i = 0; i < len; i++){
		while(USART_GetFlagStatus(USART2,USART_FLAG_TXE) != SET){
		}
		USART2->DR = *(data + i);
	}
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC) != SET){
	}
	USART_ClearFlag(USART2,USART_FLAG_TC);
}

// 回复匿名上位机一个确认帧，表示对应命令已被正确接收处理。
static void ANO_DT_Send_Check(u8 head, u8 check_sum)
{
	u8 dataToSend[10];
	u8 sum = 0;

	dataToSend[0] = 0xAA;
	dataToSend[1] = 0xAA;
	dataToSend[2] = 0xEF;
	dataToSend[3] = 2;
	dataToSend[4] = head;
	dataToSend[5] = check_sum;

	for(u8 i = 0; i < 6; i++){
		sum += dataToSend[i];
	}
	dataToSend[6] = sum;

	Usart2Send(dataToSend, 7);
}

// 解析匿名上位机发来的完整数据帧。
// 当前支持读取 PID、恢复默认 PID、写入姿态角环 PID 和角速度环 PID。
static void ANODataReceiveAnalysis(u8 *dataBuffer,u8 num)
{
	u8 sum = 0;

	for(u8 i = 0; i < (num - 1); i++){
		sum += *(dataBuffer + i);
	}

	if(!(sum == *(dataBuffer + num - 1))){
		return;
	}

	if(!(*(dataBuffer) == 0xAA && *(dataBuffer + 1) == 0xAF)){
		return;
	}

	if(*(dataBuffer + 2) == 0X02){
		if(*(dataBuffer + 4) == 0X01){
			DateTransfer();
		}
		if(*(dataBuffer + 4) == 0XA1){
			pidResetFlag = 1;
			AllPidInit();
			DateTransfer();
		}
	}

	if(*(dataBuffer + 2) == 0X10){
		allPid.rolAngle.kp = 0.01 * ((vs16)(*(dataBuffer + 4) << 8) | *(dataBuffer + 5));
		allPid.rolAngle.ki = 0.01 * ((vs16)(*(dataBuffer + 6) << 8) | *(dataBuffer + 7));
		allPid.rolAngle.kd = 0.01 * ((vs16)(*(dataBuffer + 8) << 8) | *(dataBuffer + 9));
		allPid.pitAngle.kp = 0.01 * ((vs16)(*(dataBuffer + 10) << 8) | *(dataBuffer + 11));
		allPid.pitAngle.ki = 0.01 * ((vs16)(*(dataBuffer + 12) << 8) | *(dataBuffer + 13));
		allPid.pitAngle.kd = 0.01 * ((vs16)(*(dataBuffer + 14) << 8) | *(dataBuffer + 15));
		allPid.yawAngle.kp = 0.01 * ((vs16)(*(dataBuffer + 16) << 8) | *(dataBuffer + 17));
		allPid.yawAngle.ki = 0.01 * ((vs16)(*(dataBuffer + 18) << 8) | *(dataBuffer + 19));
		allPid.yawAngle.kd = 0.01 * ((vs16)(*(dataBuffer + 20) << 8) | *(dataBuffer + 21));

		ANO_DT_Send_Check(*(dataBuffer + 2),sum);
		PidDataWriteToFlash(PID_WRITE_ADDRESS,&allPid);
	}

	if(*(dataBuffer + 2) == 0X11){
		allPid.rolGyro.kp = 0.01 * ((vs16)(*(dataBuffer + 4) << 8) | *(dataBuffer + 5));
		allPid.rolGyro.ki = 0.0001 * ((vs16)(*(dataBuffer + 6) << 8) | *(dataBuffer + 7));
		allPid.rolGyro.kd = 0.001 * ((vs16)(*(dataBuffer + 8) << 8) | *(dataBuffer + 9));
		allPid.pitGyro.kp = 0.01 * ((vs16)(*(dataBuffer + 10) << 8) | *(dataBuffer + 11));
		allPid.pitGyro.ki = 0.0001 * ((vs16)(*(dataBuffer + 12) << 8) | *(dataBuffer + 13));
		allPid.pitGyro.kd = 0.001 * ((vs16)(*(dataBuffer + 14) << 8) | *(dataBuffer + 15));
		allPid.yawGyro.kp = 0.01 * ((vs16)(*(dataBuffer + 16) << 8) | *(dataBuffer + 17));
		allPid.yawGyro.ki = 0.0001 * ((vs16)(*(dataBuffer + 18) << 8) | *(dataBuffer + 19));
		allPid.yawGyro.kd = 0.001 * ((vs16)(*(dataBuffer + 20) << 8) | *(dataBuffer + 21));

		ANO_DT_Send_Check(*(dataBuffer + 2),sum);
		PidDataWriteToFlash(PID_WRITE_ADDRESS,&allPid);
	}

	if(*(dataBuffer + 2) == 0X12 || *(dataBuffer + 2) == 0X13 ||
	   *(dataBuffer + 2) == 0X14 || *(dataBuffer + 2) == 0X15){
		ANO_DT_Send_Check(*(dataBuffer + 2),sum);
	}
}

// 按匿名上位机协议逐字节组帧：
// 帧头 AA AF，后接功能字、长度、数据区和校验和。
static void ANODTDataReceivePrepare(u8 data)
{
	static u8 receiveBuffer[ANO_RX_FRAME_BUF_LEN];
	static u8 dataLen = 0;
	static u8 cnt = 0;
	static u8 state = 0;

	if(state == 0 && data == 0xAA){
		state = 1;
		receiveBuffer[0] = data;
	}
	else if(state == 1 && data == 0xAF){
		state = 2;
		receiveBuffer[1] = data;
	}
	else if(state == 2 && data < 0XF1){
		state = 3;
		receiveBuffer[2] = data;
	}
	else if(state == 3 && data < ANO_RX_FRAME_BUF_LEN){
		state = 4;
		receiveBuffer[3] = data;
		dataLen = data;
		cnt = 0;
	}
	else if(state == 4 && dataLen > 0){
		dataLen--;
		receiveBuffer[4 + cnt++] = data;
		if(dataLen == 0){
			state = 5;
		}
	}
	else if(state == 5){
		u8 frameLen = cnt + 5;
		state = 0;
		receiveBuffer[4 + cnt] = data;

		if(!ano_frame_pending && frameLen <= ANO_RX_FRAME_BUF_LEN){
			for(u8 i = 0; i < frameLen; i++){
				ano_pending_frame[i] = receiveBuffer[i];
			}
			ano_pending_len = frameLen;
			ano_frame_pending = 1;
		}
	}
	else{
		state = 0;
	}
}

// 中断里只收字节和推进组帧状态机，避免做耗时操作。
void USART2_IRQHandler(void)
{
	u8 data = 0;

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET){
		data = USART_ReceiveData(USART2);
		ANODTDataReceivePrepare(data);
	}
}
