# Remote 遥控器代码说明

这份文档基于 `Remote/user/main.c` 和 `Remote/driver` 目录梳理，目标是帮助后续维护者快速看懂遥控器工程的启动流程、运行链路、配对逻辑、数据包格式，以及各驱动文件在整个系统里的角色。

## 1. 工程在做什么

`Remote` 目录是一套运行在 STM32F103 上的遥控器固件。它的核心职责可以概括成 4 件事：

1. 采集摇杆和按键输入。
2. 将输入转换成遥控协议数据包。
3. 通过 `NRF24L01` 无线模块周期性发给飞控端。
4. 通过带载荷的 ACK 回包更新 OLED 提示和状态灯。

从代码结构看，这个工程不是“主循环里一直发包”的写法，而是典型的“定时器触发采样，DMA 中断里完成发送”的中断驱动方案。

## 2. 先看主流程

`Remote/user/main.c` 做的事情比较集中，上电后顺序如下：

1. `SystemInit()` 初始化系统时钟。
2. `systick_init()` 初始化滴答定时器，供 `delay_us` / `delay_ms` 使用。
3. `LedInit()` 初始化 RGB 状态灯。
4. `Usart1Init(115200)` 初始化串口 1，用于调试输出。
5. `get_chip_id()` 读取芯片唯一 ID，给后面的配对地址生成做准备。
6. `SPI1_Init()` 初始化 SPI1。
7. `NRF24L01_Init()` 初始化无线模块 IO。
8. `NRF24L01_Check()` 检查无线模块是否在线，失败时红灯闪烁等待。
9. `NRF24L01_TX_Mode()` 把无线模块切到发射模式。
10. `KeyInit()` 初始化左右功能按键。
11. `ADC_Config()` 配置 4 路摇杆 ADC 和 DMA。
12. `timing_trigger_init()` 启动 TIM4，周期性触发 ADC 转换。
13. `OLED_Init()` 和 `OLED_Clear()` 初始化屏幕。
14. `NVIC_config()` 打开 `DMA1_Channel1_IRQn` 中断。

主循环本身非常轻：

```c
while(1){
    WaitPairing();
    key_info();
    OledDisplayPairStatus();
}
```

也就是说，`while(1)` 只负责：

- 根据油门杆位置推进配对状态机。
- 更新左右按键状态。
- 按当前连接/配对状态刷新 OLED。

真正的周期采样和无线发送，不在这里，而是在 DMA 完成中断里。

## 3. 运行时主链路

遥控器最关键的一条链路如下：

```text
TIM4 每 20ms 触发一次
    -> ADC1 扫描 PA0~PA3 四路模拟量
    -> DMA1_Channel1 把结果搬到 ADC_value[4]
    -> 触发 DMA1_Channel1_IRQHandler()
    -> analyze_packet() 把 ADC 原始值换算成遥控量
    -> data_exchange() 组包
    -> NRF24L01_TxPacket() 发射
    -> 读取 ACK 载荷，更新 rxPacketStatus / 飞机状态
```

这条链路分散在几个文件里：

- `timing_trigger.c`：产生固定节拍。
- `adc.c`：完成 ADC + DMA 配置。
- `sendpacket.c`：中断里完成数据换算、组包、发射、收 ACK。
- `nrf24l01.c`：完成无线收发底层操作。

## 4. 采样与发送节奏

### 4.1 TIM4 触发频率

`timing_trigger_init()` 把 `TIM4` 配成：

- 预分频：`72 - 1`
- 自动重装：`20000 - 1`
- 周期：20 ms
- 触发点：`TIM4_CH4` 在 5 ms 位置输出比较事件

结合 `adc.c` 中的 `ADC_ExternalTrigConv_T4_CC4`，可以看出：

- 遥控器以 `50 Hz` 的节奏采样并发包。
- 每次采样是由定时器硬触发的，不依赖主循环跑多快。

### 4.2 ADC 通道分配

`adc_gpio_init()` 将 `PA0 ~ PA3` 配成模拟输入，`adc_config()` 让 ADC1 以扫描模式依次采 4 路。

DMA 把 4 路结果放进：

```c
uint16_t ADC_value[4];
```

`sendpacket.c` 里对 4 路数据的解释如下：

- `ADC_value[0] -> yaw`
- `ADC_value[1] -> thr`
- `ADC_value[2] -> rol`
- `ADC_value[3] -> pit`

换算规则：

- `thr = adc * 0.24420f`，映射到约 `0 ~ 1000`
- `yaw / rol / pit = adc * 0.02442f`，映射到约 `0 ~ 100`

这说明当前协议设计里：

- 油门单独保留了更高分辨率，使用 `uint16_t`
- 另外三个姿态通道压缩成 `uint8_t`

## 5. 发包逻辑藏在哪里

工程里最重要、也最容易忽略的一点是：

`DMA1_Channel1_IRQHandler()` 被定义在 `driver/sendpacket.c`，不是写在常见的 `stm32f10x_it.c` 里。

这个中断函数做了 4 件事：

1. `analyze_packet(ADC_value)` 把本轮 ADC 原始值转换成 `tx` 结构体。
2. `data_exchange(tx_dat)` 按当前状态生成待发送数据包。
3. `NRF24L01_TxPacket(tx_dat)` 立即通过 NRF24L01 发射。
4. 根据发送结果更新 LED，并清 DMA 中断标志位。

因此，遥控器的“业务核心”其实是被 DMA 中断驱动的，而不是被主循环驱动的。

## 6. 遥控数据结构与协议格式

### 6.1 本地遥控量结构

`sendpacket.h` 里定义了本地遥控量：

```c
typedef struct
{
    uint16_t thr;
    uint8_t pit;
    uint8_t rol;
    uint8_t yaw;
    uint8_t key;
} RemoteData;
```

实际发包时，还会再组合左右按键值 `key.l` / `key.r`。

### 6.2 正常控制包

当 `pair.step == DONE` 时，`data_exchange()` 组正常控制包，长度固定为 `11` 字节：

| 字节下标 | 含义 |
| --- | --- |
| 0 | 包头 `0xA8` |
| 1 | 油门低字节 |
| 2 | 油门高字节 |
| 3 | `pit` |
| 4 | `rol` |
| 5 | `yaw` |
| 6 | 左功能键值 `key.l` |
| 7 | 右功能键值 `key.r` |
| 8 | 预留 |
| 9 | 预留 |
| 10 | 包尾 `0x8B` |

当前按键编码来自 `key_info()`：

- 左键按下：`key.l = 0xE1`
- 右键按下：`key.r = 0xC8`
- 都没按：清零

### 6.3 配对包

当 `pair.step == STEP1` 时，发送的不是正常控制量，而是“把新地址和频点告诉飞机”的配对包：

| 字节下标 | 含义 |
| --- | --- |
| 0 | 包头 `0xA8` |
| 1~5 | 新的 5 字节地址 `pair.addr[]` |
| 6 | 新频点 `pair.freq_channel` |
| 10 | 包尾 `0x8B` |

中间未显式填写的字节当前没有被使用。

## 7. 配对流程

配对相关逻辑在 `pair_freq.c`，状态机很简单：

- `NOT`：未配对
- `STEP1`：正在配对
- `DONE`：配对完成

### 7.1 初始值

系统上电后，`pair` 初始内容是：

- 地址：`{0x1F, 0x2E, 0x3D, 0x4C, 0x5B}`
- 频点：`5`
- 状态：`NOT`

也就是说，遥控器一开始会先用默认地址和默认频点工作。

### 7.2 触发配对

`WaitPairing()` 使用油门杆来触发配对：

1. 当状态为 `NOT` 且 `tx.thr > 900` 时，进入 `STEP1`。
2. 进入 `STEP1` 后，读取 `ID` 生成新的 `pair.addr[]`。
3. 同时把通信频点改成 `30`。

这里的新地址来自 `get_chip_id()` 读取的 STM32 唯一 ID。代码里把 3 个 32 位寄存器做按位或后得到 `ID`，再取其 4 个字节，加上第 1 字节重复一次，组成 5 字节地址。

### 7.3 配对完成

当处于 `STEP1` 且 `tx.thr < 100` 时，认为用户已把油门拉回低位，执行真正的重配置：

1. 拉低 `CE`
2. 把 `pair.addr` 写入 `TX_ADDR`
3. 把同样地址写入 `RX_ADDR_P0`
4. 把 `pair.freq_channel` 写入 `RF_CH`
5. 拉高 `CE`
6. 将状态置为 `DONE`

所以这套交互本质上是：

- 先把油门推高，开始广播“我要换到新地址/新频点”
- 再把油门拉低，正式切到新地址/新频点并结束配对

## 8. NRF24L01 在这里怎么用

### 8.1 引脚连接

从头文件定义可以看出，无线模块使用：

- `SPI1`：`PA5` SCK, `PA6` MISO, `PA7` MOSI
- `CSN`：`PA4`
- `CE`：`PB10`
- `IRQ`：`PB0`

### 8.2 初始化方式

`NRF24L01_TX_Mode()` 把模块配置为发射端，主要参数有：

- 地址宽度：5 字节
- 启用 `pipe0` 自动应答
- 启用动态载荷和带载荷 ACK
- 当前工作信道：`pair.freq_channel`
- 自动重发配置：`0x1a`
- 射频配置：`0x07`
- `CONFIG = 0x0e`

因此，这里不是“发完就不管”，而是利用 ACK 机制从飞机端带回状态字节。

### 8.3 ACK 回包的用途

`NRF24L01_TxPacket()` 在发包成功后会调用 `NrfRxPacket()`，读取 ACK 里的载荷：

- 包头要求为 `0xAA`
- 包尾要求为 `0xAC`

如果头尾合法，就把 `rxPacketStatus` 置为 1，表示飞控端有有效回应。

OLED 上显示的很多内容并不是遥控器本地推断的，而是依据这个 ACK 包判断的。

## 9. OLED 和状态灯分别表达什么

### 9.1 OLED 连接方式

OLED 不是硬件 I2C，而是 `iic.c` 里自己模拟出来的时序：

- `PB13`：SCL
- `PB14`：SDA

`OLED_Init()` 内部先调用 `I2cInit()`，然后写一串 SSD1306 风格初始化命令。

### 9.2 OLED 状态显示逻辑

`OledDisplayPairStatus()` 按 `rxPacketStatus + pair.step` 组合状态显示内容：

- 飞机在线，且 `NOT`：显示“未对频”和配对提示
- `STEP1`：显示“对频中”和操作提示
- 飞机在线，且 `DONE`：显示“对频完成”，并进一步显示飞机状态
- 飞机离线，且未完成配对：显示“未检测到飞机”
- 飞机离线，但已完成配对：显示“信号丢失”

`DisplayPlaneInfo()` 还会解析 ACK 里的两个状态位：

- `rxPacket[3] == 0`：锁定
- `rxPacket[3] == 1`：解锁
- `rxPacket[4] == 1`：低电量

### 9.3 LED 状态

从 `led.c` 和主流程可以看出：

- 上电初始化默认亮红灯
- `NRF24L01_Check()` 失败时红灯闪烁
- 正常发包成功时亮绿灯
- 发包失败或链路异常时黄灯闪烁

## 10. 各驱动文件分工

### 10.1 当前主流程真正依赖的模块

| 文件 | 作用 |
| --- | --- |
| `user/main.c` | 上电初始化、主循环入口 |
| `driver/systick.c` | 微秒/毫秒延时 |
| `driver/led.c` | RGB 状态灯 |
| `driver/spi.c` | SPI1，总线服务于 NRF24L01 |
| `driver/nrf24l01.c` | 无线模块寄存器配置与收发 |
| `driver/adc.c` | 摇杆 ADC 扫描与 DMA |
| `driver/timing_trigger.c` | TIM4 产生 20ms 采样节拍 |
| `driver/sendpacket.c` | ADC 数据换算、组包、DMA 中断发送、ACK 接收 |
| `driver/pair_freq.c` | 配对状态机、唯一 ID 地址生成 |
| `driver/key.c` | 左右功能键扫描 |
| `driver/oled.c` | 屏幕显示 |
| `driver/iic.c` | OLED 使用的软件 I2C |
| `driver/nvic.c` | 打开 DMA 中断 |
| `driver/usart1.c` | 串口调试输出 |

### 10.2 当前主流程里没有明显使用的模块

这些文件存在于 `driver` 中，但在当前 `main.c` 主线里没有直接参与核心遥控流程：

| 文件 | 说明 |
| --- | --- |
| `driver/filter.c` | 二阶 Butterworth 低通滤波工具 |
| `driver/imath.c` | 数学辅助函数，如反平方根、油门限幅 |
| `driver/usart3.c` | 串口 3 + DMA 发送支持 |

它们更像后续扩展时可复用的基础能力。

## 11. 关键硬件资源速查

| 功能 | 资源 |
| --- | --- |
| 四路摇杆 ADC | `PA0` `PA1` `PA2` `PA3` |
| NRF24L01 SPI | `PA5` `PA6` `PA7` |
| NRF24L01 CSN | `PA4` |
| NRF24L01 CE | `PB10` |
| NRF24L01 IRQ | `PB0` |
| OLED 软件 I2C SCL/SDA | `PB13` / `PB14` |
| 左按键 | `PC14` |
| 右按键 | `PB12` |
| RGB LED | `PB7` `PB8` `PB9` |
| USART1 调试 | `PA9` / `PA10` |

## 12. 阅读和二次开发时最值得记住的点

1. 遥控发送节奏是 `50 Hz`，主循环不是实时主线，中断才是。
2. 发包入口不是普通函数，而是 `DMA1_Channel1_IRQHandler()`。
3. 配对动作依赖油门杆上下两个阈值，不是独立按键触发。
4. OLED 的连接状态来自飞机 ACK 包，不是本地超时计数。
5. 当前协议里 `byte[8]`、`byte[9]` 还空着，后续要扩展遥控功能可以优先考虑这两个字节。

---

如果后续要继续补文档，最值得往下追的是飞控端如何解析这 11 字节包，以及 ACK 载荷中每个字节到底对应哪些飞机状态。这样就能把“遥控器发送端”和“飞控接收端”的协议说明拼成完整闭环。
