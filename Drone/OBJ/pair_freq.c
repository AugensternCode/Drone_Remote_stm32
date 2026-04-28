/*
 * 模块说明：无线对频实现，负责接收遥控器下发的配对包并切换通信参数。
 */
#include "pair_freq.h"
#include "nrf24l01.h"
#include "spi.h"
#include "parse_packet.h"

/*
 * 默认通信参数。
 * 飞控刚上电时，先监听这一组默认地址和默认信道，
 * 直到遥控器发来正式配对包后，再切换成新的参数。
 */
PairInfo pair = {{0x1F, 0x2E, 0x3D, 0x4C, 0x5B}, 5};

extern PlaneData plane;

/*
 * 等待配对包。
 *
 * 处理流程：
 * 1. 仅在 plane.pair == PAIR_NOT 时尝试配对。
 * 2. 检查 nRF24L01 是否收到新包。
 * 3. 若收到合法配对包，则取出新的地址和信道。
 * 4. 立即把无线模块切换到新的地址和信道。
 */
/*
STATUS 的位大致可以这么看：
bit7 bit6  bit5  bit4  bit3 bit2 bit1  bit0
 ?  RX_DR TX_DS MAX_RT RX_P_NO[2:0]   TX_FULL
--------------------------------------------
bit6 = RX_DR
表示“收到新数据了”
你工程里对应 RX_OK = 0x40
bit5 = TX_DS
表示“发送成功了”
你工程里对应 TX_OK = 0x20
bit4 = MAX_RT
表示“自动重发达到上限，还没发成功”
你工程里对应 MAX_TX = 0x10
bit3~bit1 = RX_P_NO
表示“当前收到的数据来自哪个接收通道 pipe”
常见值：
000 -> pipe0
001 -> pipe1
...
111 -> RX FIFO 空
bit0 = TX_FULL
表示 TX FIFO 满了
*/
void wait_pairing(void)
{
    uint8_t sta;

    if (plane.pair == PAIR_NOT)
    {
        /* 读取 STATUS，并回写清除本次中断标志。 */
        sta = SPI_Read_Reg(STATUS);  //等价于SPI_Read_Reg(NRF_READ_REG + STATUS);
        SPI_Write_Reg(NRF_WRITE_REG + STATUS, sta); //写回状态寄存器STATUS,相当于把已经置位的中断标志清除
        if (sta & RX_OK)  //判断 STATUS 的 RX_DR/RX_OK 标志位是否为 1
        {
            /* 读取接收到的配对包，并清空 RX FIFO。 */
            SPI_Read_Buf(RD_RX_PLOAD, Rx_packet, RX_PLOAD_WIDTH);  //这里是主机通过SPI读从机NRF接收缓存区的数据
			/*
			真正的意思不是“往某个寄存器写 0xff”，而是：
			先发命令字 0xE2
			顺手再发一个无意义的占位字节 0xFF
			nRF24L01 收到 FLUSH_RX 命令后，把 RX FIFO 清空
			这里的 0xFF 基本就是“凑一次 SPI 传输”，重点是前面的 0xE2。
			*/
		//FIFO = First In First Out，先进先出队列。 清 FIFO 靠的是 0xE1 / 0xE2 这个命令字，不是靠 0xff。0xff 只是 SPI 通信里的陪跑字节。
            SPI_Write_Reg(FLUSH_RX, 0xff);
            /* 包尾 0x8B 作为合法帧标志。 */
            if (Rx_packet[10] == 0x8B)
            {
                pair.addr[0] = Rx_packet[1];
                pair.addr[1] = Rx_packet[2];
                pair.addr[2] = Rx_packet[3];
                pair.addr[3] = Rx_packet[4];
                pair.addr[4] = Rx_packet[5];
                pair.freq_channel = Rx_packet[6];
                plane.pair = PAIR_NORMAL;
            }
        }
        if (plane.pair == PAIR_NORMAL)
        {
            /*
             * 配对成功后重新写入接收地址和工作信道。
             * CE 先拉低，写完配置后再拉高，避免切换过程不稳定。
			先把无线模块暂停一下
			改接收地址
			改工作信道
			再重新使能，让它按新参数工作
             */
            NRF_CE_L;
            SPI_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, (u8 *)pair.addr, RX_ADR_WIDTH); //pair.addr 本来就是 u8 addr[5]，那 (u8*)强转其实不是必须的，
            SPI_Write_Reg(NRF_WRITE_REG + RF_CH, pair.freq_channel);
            NRF_CE_H;
        }
    }
}
/*
等待配对逻辑：
if 未配对：
	读取状态寄存器状态
	状态回写，清除中断标志位
	if 数据被接收（sta&RX_OK)：
		读取状态包到数组
		清除FIFO
		if 帧尾是合法的：
			更新通讯地址
			更新通讯信道
			更新配对状态
	if 配对正常：
		使能NRF_CE_L)(低电平有效)
		更新接收地址寄存器
		更新射频通道寄存器
		失能NRF_CE_H
*/

/*
Rx_packet[0]   -> 包头 / 类型 / 标识
Rx_packet[1]   -> 地址第 1 字节
Rx_packet[2]   -> 地址第 2 字节
Rx_packet[3]   -> 地址第 3 字节
Rx_packet[4]   -> 地址第 4 字节
Rx_packet[5]   -> 地址第 5 字节
Rx_packet[6]   -> 信道
...
Rx_packet[10]  -> 包尾 0x8B
------------------------------------------
NRF_CE_L;
写配置;
NRF_CE_H;
意思是：
先让无线模块退出激活的 RX/TX 状态
再改关键配置
改完再重新进入工作状态
而 STATUS 这种“中断标志寄存器”不同，它更像是在运行中随时读写的状态位：
sta = SPI_Read_Reg(STATUS);
SPI_Write_Reg(NRF_WRITE_REG + STATUS, sta);
这个操作通常不需要为了它专门拉低 CE。
你可以粗略记成：
读状态、清中断标志：通常不用动 CE
改模式、改地址、改信道：通常先 CE_L 再改
*/
/*
为什么不一直用默认地址？
主要有几个原因。
第一，默认地址只是“配对入口”
如果一直不改，那所有设备都会永远待在同一个默认地址上，谁都能收到谁的包，容易串台。
第二，正式地址相当于“这对遥控器和飞控的专属频道”
配对成功以后，双方就切到自己的地址和信道，后续通信更稳定，也更不容易和别的设备冲突。
第三，协议本来就是这么设计的
配对包的作用之一就是“下发正式通信参数”。
所以默认地址不是没用，它的作用是：
先让你能收到“配对包”
收到后再拿到真正地址
*/
/*
在对频包里，Rx_packet[1]~Rx_packet[5] 是地址，Rx_packet[6] 是信道；
但在正常控制包里，这几个字节又表示别的控制数据。
*/

/*
遥控器上电 ->（可增加特定动作）-> 发配对包 -> 无人机接收 -> 切换专属地址/信道 -> 开始正常控制
*/