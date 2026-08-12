#include "pair_freq.h"
#include "sendpacket.h"
#include "nrf24l01.h"
#include "spi.h"

Pair pair = {0,{0x1F,0x2E,0x3D,0x4C,0x5B},5};//配对标志位；初始接收地址；初始接收频点

uint32_t  chip_id[3] = {0};  

uint32_t ID = 0;

//读取芯片ID
void get_chip_id(void)
{
    chip_id[0] = *(__IO u32 *)(0X1FFFF7F0); // 高字节
    chip_id[1] = *(__IO u32 *)(0X1FFFF7EC); //
    chip_id[2] = *(__IO u32 *)(0X1FFFF7E8); // 低字节
    
    ID = (chip_id[0] | chip_id[1] | chip_id[2]); 
}


//对频
void WaitPairing(void)
{
    if(pair.step == NOT && tx.thr>900){		//油门杆向上推进行对频    
			pair.step = STEP1;//对频标志位
			
			//地址装入对频待发送数组
			pair.addr[0] = *((uint8_t *)&ID);		 
			pair.addr[1] = *(((uint8_t *)&ID)+1);	     
			pair.addr[2] = *(((uint8_t *)&ID)+2);		
			pair.addr[3] = *(((uint8_t *)&ID)+3);		
			pair.addr[4] = pair.addr[0];			
			
		 //频点装入数组
			pair.freq_channel = 30;
		}
		
		if(tx.thr < 100 && pair.step == STEP1){		//油门杆下拉，重置对频地址
			Clr_NRF24L01_CE();	
			
			//写入新的地址和地址
			NRF24L01_Write_Buf(SPI_WRITE_REG+TX_ADDR,(uint8_t*)pair.addr,TX_ADR_WIDTH);    //写TX节点地址 
			NRF24L01_Write_Buf(SPI_WRITE_REG+RX_ADDR_P0,(uint8_t*)pair.addr,RX_ADR_WIDTH); //设置TX节点地址,主要为了使能ACK	 
			NRF24L01_Write_Reg(SPI_WRITE_REG+RF_CH,pair.freq_channel);                  //设置RF通道（频点）
			
			Set_NRF24L01_CE();                                          //CE为高,10us后启动发送
			pair.step = DONE;				
		}            
}

/*
在改这些寄存器之前，必须先 Clr_NRF24L01_CE() 把 CE 拉低，目的是：
1. 让芯片回到待机状态，保证 SPI 写寄存器操作稳定可靠——很多 NRF24L01 的操作要求在 CE 为低时改配置才生效/才安全。
2. 避免在改地址/频点的半途，芯片还在用旧配置发波，导致发出一串错乱的中间状态帧。
*/

// ① 油门上推(thr > 900), step : NOT→STEP1 遥控器把 pair.addr / freq 装进 tx_dat，通过无线发给无人机
//    （此时遥控器自己还没改本地地址，仍用旧地址通信）
// ② 无人机收到 STEP1 的对频包(帧头0xA8)，解析出 addr + freq， 写入自己的 TX_ADDR / RX_ADDR_P0 / RF_CH，完成 "换身份"

// ③ 油门下拉(thr < 100), step : STEP1→DONE 遥控器本地执行 Clr_CE → 写 TX_ADDR / RX_ADDR_P0 / RF_CH → Set_CE 遥控器 "自己" 也换成新地址 + 新频点

// ④ step = DONE，之后双方都用新地址 + 新频点正常通信
