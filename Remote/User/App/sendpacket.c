#include "sendpacket.h"
#include "timing_trigger.h"
#include "usart1.h"
#include "adc.h"
#include "nrf24l01.h"
#include "key.h"
#include "led.h"
#include "pair_freq.h"
#include "oled.h"
#include "systick.h"
#include <string.h>

RemoteData tx = {0};
extern Pair pair;

static u8 rxPacket[TX_PLOAD_WIDTH] = {0};
volatile u8 rxPacketStatus;
volatile uint32_t lastValidAckTick;
volatile uint32_t g_radio_tx_ok;
volatile uint32_t g_radio_tx_fail;
volatile uint32_t g_radio_tx_timeout;
volatile uint8_t g_radio_last_result;



void analyze_packet(volatile uint16_t *adcData)
{
    //固定翼遥控打舵方向数据转为 0 ~ 100
		//油门转为 0 ~ 1000
    tx.thr = (uint16_t)(((float)(adcData[1]))*0.24420f);
    tx.yaw = (uint8_t)(((float)(adcData[0]))*0.02442f);
    tx.pit = (uint8_t)(((float)(adcData[3]))*0.02442f);
    tx.rol = (uint8_t)(((float)(adcData[2]))*0.02442f);
}

void data_exchange(uint8_t *dateBuff)
{
    /* NOT: a zero probe without 0x8B cannot trigger Drone wait_pairing(). */
    memset(dateBuff, 0, TX_PLOAD_WIDTH);
	//发送新的地址和频点给飞机
	if(pair.step == STEP1){
			dateBuff[0] = 0xA8;					//帧头
			dateBuff[1] = pair.addr[0];		  //5个字节地址    
			dateBuff[2] = pair.addr[1];	    //    
			dateBuff[3] = pair.addr[2];			//	
			dateBuff[4] = pair.addr[3];			//	
			dateBuff[5] = pair.addr[4];			//	
			dateBuff[6] = pair.freq_channel;		//通信频点		
			dateBuff[TX_PLOAD_WIDTH-1] = 0x8B;	//帧尾      
	}
	//正常数据通信
	else if(pair.step == DONE){
			dateBuff[0] = 0xA8;		//帧头
			dateBuff[1] = *((u8*)&tx.thr);		//油门低八位
			dateBuff[2] = *(((u8*)&tx.thr)+1);		//油门高八位
			dateBuff[3] = tx.pit;		//俯仰舵向
			dateBuff[4] = tx.rol;		//横滚舵向
			dateBuff[5] = tx.yaw;		//偏航舵向
			dateBuff[6] = key.l;		//左功能按键
			dateBuff[7] = key.r;		//右功能按键
			dateBuff[8] = 0;		//未使用，可自行添加
			dateBuff[9] = 0;		//未使用，可自行添加
			dateBuff[TX_PLOAD_WIDTH-1] = 0x8B;		//帧尾     
	}
}

uint8_t tx_dat[TX_PLOAD_WIDTH] = {0};


//DMA通道1中断触发，如果触发，则表明DMA已经将ADC数据搬运到内存数组中，则可以进行发送数据了    
void DMA1_Channel1_IRQHandler(void)
{
	uint8_t ret=0;
	
	if(DMA_GetITStatus(DMA1_IT_TC1) == SET){	
		analyze_packet(ADC_value);		
		data_exchange(tx_dat);//数据装入发送数组

		ret=NRF24L01_TxPacket(tx_dat);//发送数据
		g_radio_last_result = ret;
		if(ret == TX_OK){
            ++g_radio_tx_ok;
			LedColorSet(GREEN);		//发送成功绿灯亮
		} else{
            ++g_radio_tx_fail;
            if (ret == NRF24L01_TIMEOUT) ++g_radio_tx_timeout;
			LedBlink(YELLOW);		//信号丢失黄灯闪烁
		}
		
		DMA_ClearITPendingBit(DMA1_IT_TC1);		
		
	}
}

//接收数据包
void NrfRxPacket(void)
{
    uint8_t status, fifo, width;
    uint8_t packet[RX_PLOAD_WIDTH];
    NrfInvalidateRx();
    status = NRF24L01_Read_Reg(STATUS);
    fifo = NRF24L01_Read_Reg(FIFO_STATUS);
    if ((status == NRF24L01_ERR) || (fifo == NRF24L01_ERR) ||
        ((status & RX_OK) == 0U) || ((fifo & 1U) != 0U)) {
        return;
    }
    if ((NRF24L01_Read_Buf(R_RX_PL_WID, &width, 1) == NRF24L01_ERR) ||
        (width != RX_PLOAD_WIDTH)) {
        NRF24L01_Write_Buf(FLUSH_RX, NULL, 0);
        return;
    }
    if (NRF24L01_Read_Buf(RD_RX_PLOAD, packet, width) == NRF24L01_ERR) {
        NRF24L01_Write_Buf(FLUSH_RX, NULL, 0);
        return;
    }
    if ((packet[0] == 0xAA) && (packet[RX_PLOAD_WIDTH - 1] == 0xAC)) {
        memcpy(rxPacket, packet, sizeof(rxPacket));
        lastValidAckTick = GetTick();
        rxPacketStatus = 1;
    }
}

void NrfInvalidateRx(void)
{
    rxPacketStatus = 0;
}

void DisplayPlaneInfo(void)
{
	static u8 clear = 0;
	
	if(clear != ((rxPacket[1]<<4)|rxPacket[2])){
		OLED_Clear();
		clear = (rxPacket[1]<<4)|rxPacket[2];
	}
		
	if(rxPacket[1] == 0){
		OledDisplayChinese(3,3,lock,2);//锁定
	}
	else if(rxPacket[1] == 1){
		OledDisplayChinese(3,3,unlock,2);//解锁
	}
	
	if(rxPacket[2] == 1){
		OledDisplayChinese(6,2,powerLow,3);//电量低
	} 
}

//屏幕显示
void OledDisplayPairStatus(void)
{
	static u8 clearDetect = 0;
    if ((uint32_t)(GetTick() - lastValidAckTick) >= 200U) {
        NrfInvalidateRx();
    }
	if(clearDetect != ((rxPacketStatus<<4)|pair.step)){
		OLED_Clear();
		clearDetect = (rxPacketStatus<<4)|pair.step;
	}
	
	if(rxPacketStatus == 1 && pair.step == NOT){		
		OledDisplayChinese(0,2,pairNot,3);//未对频
		OledDisplayChinese(3,0,pairTips1,9);//左遥感上推进行对频
	}
	else if(pair.step == STEP1){
		OledDisplayChinese(0,2,pairing,3);//对频中
		OledDisplayChinese(3,0,pairTips2,9);//左摇杆下拉完成对频
	}
	else if(rxPacketStatus == 1 && pair.step == DONE){
		OledDisplayChinese(0,2,pairDone,4);//对频完成
		DisplayPlaneInfo();
	}
	else if(rxPacketStatus == 0 && pair.step != DONE){		
		OledDisplayChinese(0,1,NoPlaneDetected,6);//未检测到飞机
		OledDisplayChinese(3,0,planeIfOn,16);
	}
	else if(rxPacketStatus == 0 && pair.step == DONE){		
		OledDisplayChinese(0,2,signalLost,4);//信号丢失
	}
}

