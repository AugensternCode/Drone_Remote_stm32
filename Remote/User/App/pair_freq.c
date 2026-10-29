#include "pair_freq.h"
#include "sendpacket.h"
#include "nrf24l01.h"
#include "spi.h"
//配对标志位；初始接收地址；初始接收频点
Pair pair = {NOT,{0x1F,0x2E,0x3D,0x4C,0x5B},5};
uint32_t  chip_id[3] = {0};  
uint32_t ID = 0;

//读取芯片ID 96位的地址存储
void get_chip_id(void)
{
    chip_id[0] = *(__IO u32 *)(CHIP_ID_ADRESS+8); // 高32位
    chip_id[1] = *(__IO u32 *)(CHIP_ID_ADRESS+4); // 中32位
    chip_id[2] = *(__IO u32 *)(CHIP_ID_ADRESS+0); // 低32位
    ID = (chip_id[0] ^ chip_id[1] ^ chip_id[2]); 
}
static void Pairaddr_init(uint32_t id)
{
	pair.addr[0]=(uint8_t)(id>>0);
	pair.addr[1]=(uint8_t)(id>>8);
	pair.addr[2]=(uint8_t)(id>>16);
	pair.addr[3]=(uint8_t)(id>>24);
	pair.addr[4]=pair.addr[0];
}
void WaitPairing(void)
{
	switch(pair.step)
	{
		case NOT:
			if(tx.thr>900)
			{
				pair.step=STEP1;
				Pairaddr_init(ID);
				pair.freq_channel=30;
			}
			break;
		case STEP1:
			if(tx.thr<100)
			{
				//CE 拉低 → 芯片进入 Standby-I / 待机模式，不发射、不接收。可修改配置
				Clr_NRF24L01_CE();				
				NRF24L01_Write_Buf(SPI_WRITE_REG+TX_ADDR,pair.addr,TX_ADR_WIDTH);
				NRF24L01_Write_Buf(SPI_WRITE_REG+RX_ADDR_P0,pair.addr,RX_ADR_WIDTH);
				NRF24L01_Write_Reg(SPI_WRITE_REG+RF_CH,pair.freq_channel);
				Set_NRF24L01_CE();
				pair.step=DONE;
			}
			break;
		case DONE:
			break;
		default:
			pair.step=NOT;
			break;
	}
}
