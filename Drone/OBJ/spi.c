/*
 * 模块说明：SPI1 底层读写实现，主要服务于 nRF24L01。
 */
#include "spi.h"

/*
 * 初始化 SPI1。
 *
 * 引脚分配：
 * PA5 -> SCK
 * PA6 -> MISO
 * PA7 -> MOSI
 * PB0 -> CSN（由软件控制）
 */
void SpiInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef SPI_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_AFIO, ENABLE);

    /* SCK 和 MOSI 配置为复用推挽输出。 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* MISO 配置为浮空输入。 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* CSN 由软件控制，因此配置为普通推挽输出。 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
}

/*
主机等待发送缓存区为空，然后把数据装入发送缓存区；SPI 硬件在主机时钟驱动下把数据发给从机，
同时从机也会回一个字节到主机接收缓存区；当主机接收缓存区有 1 个字节后，RXNE 置位；
最后主机通过 SPI_I2S_ReceiveData(SPI1) 读取接收缓存区的数据并返回。
 */
u8 Spi_RW_Byte(u8 TxData)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET); //等待发送缓冲区为空。
    SPI_I2S_SendData(SPI1, TxData);  //把 TxData 写入 SPI1 的数据寄存器，SPI 硬件开始把这个字节通过 MOSI 发出去。
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET); //等待接收缓冲区非空。
    //SPI 每发送 1 bit，通常也会从 MISO 同步接收 1 bit。等 8 bit 发完以后，接收缓冲区里就有 1 个字节了，RXNE 会置位
    return SPI_I2S_ReceiveData(SPI1); //读取 SPI1 接收到的数据，并返回。
}
