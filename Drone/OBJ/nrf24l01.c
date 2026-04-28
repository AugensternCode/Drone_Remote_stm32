/*
 * 模块说明：nRF24L01 无线驱动实现，负责收发模式切换、寄存器访问与数据收发。
 */
#include "nrf24l01.h"
#include "spi.h"
#include "systick.h"
#include "led.h"
#include "imath.h"
#include "pair_freq.h"

const u8 TX_ADDRESS[TX_ADR_WIDTH] = {0x1F,0x2E,0x3D,0x4C,0x5B};
const u8 RX_ADDRESS[RX_ADR_WIDTH] = {0x1F,0x2E,0x3D,0x4C,0x5B};

/*
 * 初始化 nRF24L01 相关控制引脚。
 *
 * SPI 外设本身由 spi.c 初始化，
 * 这里主要负责 CE、IRQ 和默认片选电平。
 */
void NRF24L01Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA, ENABLE);

    /* CE 引脚：控制收发状态切换。 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* IRQ 引脚：nRF24L01 中断输出。 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    NRF_CE_L;
    SPI_CSN_H;
}

/*
 * 通过“写入 TX_ADDR 再读回”的方式检查无线芯片是否在线。
 *
 * 返回 0 表示检查成功，返回 1 表示失败。
 */
u8 NRF24L01_Check(void)
{
    u8 buf[5] = {0X18,0X18,0X18,0X18,0X18};
    u8 i;

    SPI_Write_Buf(NRF_WRITE_REG + TX_ADDR, buf, 5);
    SPI_Read_Buf(TX_ADDR, buf, 5);

    for (i = 0; i < 5; i++)
    {
        if (buf[i] != 0X18)
        {
            break;
        }
    }

    if (i != 5)
    {
        return 1;
    }

    return 0;
}

/* 写单字节寄存器，返回首字节读回的 STATUS。 */
u8 SPI_Write_Reg(u8 reg,u8 value)
{
    u8 status;
    SPI_CSN_L;
    status = Spi_RW_Byte(reg);
    Spi_RW_Byte(value);
    SPI_CSN_H;
    return status;
}

/* 读取单字节寄存器内容。 */
u8 SPI_Read_Reg(u8 reg)
{
    u8 reg_val;
    SPI_CSN_L;
    Spi_RW_Byte(reg);  //其实这里就是返回了状态寄存器的值
    reg_val = Spi_RW_Byte(0XFF);
    /*
    第一个 STATUS：从机说“我收到你的命令了，顺便告诉你我当前状态”
    第二个 STATUS：从机说“你要读的那个 STATUS 寄存器内容是这个”
    */
    SPI_CSN_H;
    return reg_val;
}


/* 连续读取多个字节，常用于读取地址或 payload。 */
u8 SPI_Read_Buf(u8 reg,u8 *pBuf,u8 len)
{
    u8 status,u8_ctr;
    SPI_CSN_L;
    status = Spi_RW_Byte(reg);
    for (u8_ctr = 0; u8_ctr < len; u8_ctr++)
    {
        pBuf[u8_ctr] = Spi_RW_Byte(0XFF);
    }
    SPI_CSN_H;
    return status;
}

/* 连续写入多个字节，常用于写地址或 payload。 */
u8 SPI_Write_Buf(u8 reg, u8 *pBuf, u8 len)
{
    u8 status,u8_ctr;

    SPI_CSN_L;
    status = Spi_RW_Byte(reg);
    for (u8_ctr = 0; u8_ctr < len; u8_ctr++)
    {
        Spi_RW_Byte(*pBuf++);
    }
    SPI_CSN_H;

    return status;
}

/*
 * 切换到接收模式。
 *
 * 接收地址和工作频点来自当前对频结果 pair，
 * 这样飞控对频成功后就能监听新的无线参数。
 */
void NRF24L01ReceiveMode(void)
{
    NRF_CE_L;

    SPI_Write_Reg(SETUP_AW, 0x03);
    SPI_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, (u8*)pair.addr, RX_ADR_WIDTH);
    SPI_Write_Reg(NRF_WRITE_REG + FEATURE, 0x06);
    SPI_Write_Reg(NRF_WRITE_REG + DYNPD, 0x01);
    SPI_Write_Reg(NRF_WRITE_REG + EN_AA, 0x01);
    SPI_Write_Reg(NRF_WRITE_REG + EN_RXADDR, 0x01);
    SPI_Write_Reg(NRF_WRITE_REG + RF_CH, pair.freq_channel);
    SPI_Write_Reg(NRF_WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH);
    SPI_Write_Reg(NRF_WRITE_REG + RF_SETUP, 0x07);
    SPI_Write_Reg(NRF_WRITE_REG + CONFIG, 0x0f);

    NRF_CE_H;
}

/*
 * 尝试读取一包接收数据。
 * 返回 0 表示成功读取到新包，
 * 返回 1 表示当前没有新包。
 */
u8 NRF24L01_RxPacket(u8 *rxbuf)
{
    u8 sta;
    sta = SPI_Read_Reg(NRF_READ_REG + STATUS);
    SPI_Write_Reg(NRF_WRITE_REG + STATUS, sta);
    if (sta & RX_OK)
    {
        SPI_Read_Buf(RD_RX_PLOAD, rxbuf, RX_PLOAD_WIDTH);
        SPI_Write_Reg(FLUSH_RX, 0xff);
        return 0;
    }
    return 1;
}

/*
 * 切换到发送模式。
 *
 * TX_ADDR 和 RX_ADDR_P0 都写成同一个地址，
 * 是为了发送完成后能够收到对端返回的 ACK。
 */
void NRF24L01_TX_Mode(void)
{
    NRF_CE_L;

    SPI_Write_Reg(SETUP_AW, 0x03);
    SPI_Write_Buf(NRF_WRITE_REG + TX_ADDR, (uint8_t*)pair.addr, TX_ADR_WIDTH);
    SPI_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, (uint8_t*)pair.addr, RX_ADR_WIDTH);
    SPI_Write_Reg(NRF_WRITE_REG + EN_AA, 0x01);
    SPI_Write_Reg(NRF_WRITE_REG + EN_RXADDR, 0x01);
    SPI_Write_Reg(NRF_WRITE_REG + RF_CH, pair.freq_channel);
    SPI_Write_Reg(NRF_WRITE_REG + SETUP_RETR, 0x1a);
    SPI_Write_Reg(NRF_WRITE_REG + RF_SETUP, 0x07);
    SPI_Write_Reg(NRF_WRITE_REG + CONFIG, 0x0e);

    NRF_CE_H;
}

/*
 * 发送一包数据并等待发送结束。
 *
 * 返回值：
 * 1. TX_OK  表示发送成功。
 * 2. MAX_TX 表示达到最大自动重发次数。
 * 3. 0xFF   表示其他原因导致失败。
 */
uint8_t NRF24L01_TxPacket(uint8_t *sendBuff)
{
    uint8_t state;

    NRF_CE_L;
    SPI_Write_Buf(WR_TX_PLOAD, sendBuff, TX_PLOAD_WIDTH);
    NRF_CE_H;

    while (NRF_IRQ != 0);

    state = SPI_Read_Reg(NRF_WRITE_REG + STATUS);
    SPI_Write_Reg(NRF_WRITE_REG + STATUS, state);

    if (state & MAX_TX)
    {
        SPI_Write_Reg(FLUSH_TX, 0xff);
        return MAX_TX;
    }

    if (state & TX_OK)
    {
        return TX_OK;
    }

    return 0xff;
}
