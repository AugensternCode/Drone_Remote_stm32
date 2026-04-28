# AIFei 飞控固件项目说明

## 1. 项目简介

这是一个基于 `STM32F103C8` 的飞控固件工程，使用 `Keil uVision/MDK` 作为主要开发环境，底层依赖 `CMSIS` 和 `STM32F10x Standard Peripheral Library`。

从当前代码配置来看，这个工程默认编译为：

- `FOUR_AXIS_UAV = 1`
- 即：四轴空心杯无人机控制程序

工程已经实现了飞控最核心的一条链路：

- 无线遥控接收
- 姿态解算
- 双环 PID 控制
- 四路电机 PWM 输出
- 飞机解锁/上锁
- 电池电压检测
- 串口上位机调参
- Flash 参数保存

如果后续要切换为固定翼或无刷四轴，入口在 [USER/precompile.h](USER/precompile.h)。

## 2. 这个项目是怎么跑起来的

### 上电初始化流程

主入口在 [USER/main.c](USER/main.c)。

上电后主要做了这些事情：

1. 初始化系统滴答定时器 `SystickInit()`
2. 初始化状态灯 `RGB_LedInit()`
3. 初始化串口 2 `Usart2Init(115200)`
4. 初始化 PWM 输出 `PwmInit()`
5. 初始化软件 I2C 总线 `IIC_Init()`
6. 初始化 `MPU6050`
7. 初始化 `MPU6050 DMP`
8. 初始化 SPI 与 `NRF24L01`
9. 切换 NRF 到接收模式
10. 初始化 ADC 用于电压检测
11. 初始化 PID 参数 `AllPidInit()`
12. 初始化 `TIM3`
13. 初始化中断优先级配置 `NVIC_config()`

初始化完成后，`while(1)` 主循环里只保留了低频任务：

- `Usart2Task()` 处理串口上位机命令
- `VoltageDetect()` 电压检测

也就是说，这个工程真正的飞控控制主流程并不在 `while(1)` 里，而是在定时器中断里执行。

### 控制主循环

核心周期任务在 [OBJ/timer.c](OBJ/timer.c) 的 `TIM3_IRQHandler()` 中。

`TIM3` 被配置为：

- 计数周期约 `5ms`
- 控制频率约 `200Hz`

每 5ms 执行一次的任务顺序大致如下：

1. `wait_pairing()` 处理遥控配对
2. `NrfACKPacket()` 回传飞控状态到遥控器
3. `nrf_parse_packet()` 解析遥控数据包
4. `GyroDataTransformDeg()` 转换陀螺仪角速度单位
5. `IMU()` 获取姿态角 `roll/pitch/yaw`
6. `ControModel()` 选择控制模式并生成期望值
7. `_controller_perform()` 执行姿态外环和角速度内环 PID
8. `ControllerOutput()` 混控并输出到四路 PWM
9. `PlaneLockStatus()` 处理解锁/上锁逻辑
10. `RGB_LedStatus()` 刷新状态灯
11. `ANOSendStatus()` 按分频周期向匿名上位机发送姿态与状态

这也是理解整个工程最重要的一条线。

## 3. 控制链路说明

### 遥控输入

遥控数据由 `NRF24L01` 接收，解析逻辑在 [OBJ/parse_packet.c](OBJ/parse_packet.c)。

解析后的数据会写入全局结构体 `PlaneData plane`，其中包括：

- `throttle` 油门
- `pit / rol / yaw` 俯仰、横滚、偏航输入
- `lock` 锁定状态
- `signal` 信号状态
- `power` 电量状态
- `pair` 配对状态
- `voltage` 电池电压

对空心杯四轴模式，遥控输入会经过：

- 限幅
- 回中死区处理
- 丢包保护计数

### 姿态控制

姿态控制代码在 [OBJ/controller.c](OBJ/controller.c)。

当前主要使用的是经典双环结构：

- 外环：姿态角环 `AngleController()`
- 内环：角速度环 `GyroController()`

控制流程是：

1. 遥控器给出目标姿态角
2. 姿态角 PID 输出目标角速度
3. 角速度 PID 输出控制量
4. 将 `pitch / roll / yaw` 三轴输出与油门混合
5. 得到四个电机的最终 PWM

对于四轴模式，混控输出如下：

- `Motor1 = throttle + pit - rol - yaw`
- `Motor2 = throttle + pit + rol + yaw`
- `Motor3 = throttle - pit + rol - yaw`
- `Motor4 = throttle - pit - rol + yaw`

### 保护逻辑

当前工程已经有几类基本保护：

- 姿态异常保护：`roll` 或 `pitch` 超过 `80` 度直接上锁
- 油门较低时清空 PID 积分
- 信号丢失后逐步降低油门
- 低电压状态灯提示

### 解锁/上锁逻辑

逻辑在 [OBJ/fc_status.c](OBJ/fc_status.c)。

空心杯四轴默认解锁条件是：

- 油门 `<= 10`
- 偏航 `<= -25`
- 俯仰 `>= 25`
- 横滚 `<= -25`
- 持续约 `600` 个控制周期

由于控制周期约为 `5ms`，因此解锁动作大约需要持续 `3s`。

上锁条件包括：

- 油门低位保持很长时间
- 或油门低位加偏航打到另一侧保持一段时间

## 4. PID 与参数存储

PID 结构定义在 [OBJ/pid.h](OBJ/pid.h)，初始化逻辑在 [OBJ/pid.c](OBJ/pid.c)。

当前已经定义了多组 PID：

- 姿态角环：`pitAngle / rolAngle / yawAngle`
- 角速度环：`pitGyro / rolGyro / yawGyro`
- 预留的定高、定点 PID

默认 PID 参数写在 `pidIintData` 数组里。启动时程序会：

1. 先加载默认 PID
2. 再检查 Flash 中是否已有保存值
3. 如果存在有效值，则从 Flash 读取覆盖
4. 否则把默认值写入 Flash

### Flash 存储位置

相关代码在 [OBJ/flash.c](OBJ/flash.c) 和 [OBJ/flash.h](OBJ/flash.h)。

目前能确认的持久化区域有：

- `0x0800F000` 附近：校准数据使用
- `0x0800F400`：PID 参数页 `PID_WRITE_ADDRESS`

这两个地址都位于 `STM32F103C8` 64KB Flash 的末尾区域，属于典型的参数页保存做法。

## 5. 串口上位机与调参

串口相关代码在 [OBJ/usart2.c](OBJ/usart2.c)。

这个工程支持：

- 向匿名上位机发送姿态状态 `ANOSendStatus()`
- 接收匿名上位机的 PID 读写命令
- 在线修改 PID 后写回 Flash

默认串口配置：

- 串口：`USART2`
- 波特率：`115200`

如果你要调 PID，重点看下面几个函数：

- `ANOSendStatus()`
- `Usart2Task()`
- `PidDataWriteToFlash()`
- `PidDataReadFromFlash()`

## 6. 无线通信与配对

无线链路基于 `NRF24L01`，相关代码：

- [OBJ/nrf24l01.c](OBJ/nrf24l01.c)
- [OBJ/parse_packet.c](OBJ/parse_packet.c)
- [OBJ/pair_freq.c](OBJ/pair_freq.c)

当前逻辑包含两部分：

- 正常接收遥控器发来的 11 字节数据包
- 在未配对状态下等待配对包，更新地址和频点

默认配对信息：

- 地址：`1F 2E 3D 4C 5B`
- 频点：`5`

配对完成后会把新地址和频点写回 `NRF24L01` 的接收配置。

飞控还会通过 ACK 载荷回传基础状态，例如：

- 锁定状态
- 电源状态
- 当前电压

## 7. 已确认的硬件接口

根据当前源码，已经能确认的引脚如下。

| 功能 | 外设/引脚 | 位置 |
| --- | --- | --- |
| 电池电压采样 | `ADC1_CH1 / PA1` | `OBJ/adc.c` |
| 上位机串口 | `USART2_TX PA2` / `USART2_RX PA3` | `OBJ/usart2.c` |
| MPU6050 软件 I2C | `PB6 SCL` / `PB5 SDA` | `OBJ/iic.h` |
| NRF24L01 SPI | `PA5 SCK` / `PA6 MISO` / `PA7 MOSI` | `OBJ/spi.c` |
| NRF24L01 CSN | `PB0` | `OBJ/nrf24l01.h` |
| NRF24L01 CE | `PB1` | `OBJ/nrf24l01.h` |
| NRF24L01 IRQ | `PA4` | `OBJ/nrf24l01.h` |
| 四路 PWM 输出 | `TIM1 CH1~CH4 / PA8~PA11` | `OBJ/pwm.c` |

LED 还占用了若干 `PA/PB` 引脚，具体见 [OBJ/led.c](OBJ/led.c)。

## 8. 工程目录说明

### `USER/`

工程入口和 Keil 工程文件所在目录。

- `main.c`：主函数
- `precompile.h`：机型选择开关
- `PerFei.uvprojx`：Keil 工程文件
- `Objects/`、`Listings/`：编译输出

### `OBJ/`

这里不是“目标文件”目录，而是业务功能源码目录。飞控的大部分核心逻辑都在这里：

- `controller.*`：控制器与混控输出
- `pid.*`：PID 结构与初始化
- `imu.*`：姿态解算
- `mpu6050.*`、`inv_mpu*`：IMU 驱动与 DMP
- `nrf24l01.*`：无线通信
- `parse_packet.*`：遥控数据解析
- `fc_status.*`：锁定状态管理
- `flash.*`：参数持久化
- `adc.*`：电压采样
- `timer.*`：主控制定时中断
- `led.*`：状态灯逻辑

### `CMSIS/`

ARM Cortex-M3 和 STM32F10x 启动文件、系统文件、中断向量表。

### `LIB/`

STM32F10x 标准外设库源码。

## 9. 构建方式

当前工程明显是按 `Keil MDK-ARM` 组织的。

### 开发环境

- IDE：Keil uVision5
- 编译器：`ARMCC 5`
- Device Pack：`Keil.STM32F1xx_DFP`
- 芯片：`STM32F103C8`

### 打开工程

直接打开：

- [USER/PerFei.uvprojx](USER/PerFei.uvprojx)

### 编译输出

当前配置会生成：

- `USER/Objects/flycontroller.axf`

默认没有开启 Hex 输出，如果需要 `.hex`，可以在 Keil 的 target options 里勾选生成。

## 10. 建议从哪些文件开始看

如果你是第一次接手这个项目，推荐按下面顺序阅读：

1. [USER/main.c](USER/main.c)
2. [OBJ/timer.c](OBJ/timer.c)
3. [OBJ/parse_packet.h](OBJ/parse_packet.h) 和 [OBJ/parse_packet.c](OBJ/parse_packet.c)
4. [OBJ/controller.c](OBJ/controller.c)
5. [OBJ/pid.c](OBJ/pid.c)
6. [OBJ/imu.c](OBJ/imu.c)
7. [OBJ/flash.c](OBJ/flash.c)
8. [OBJ/usart2.c](OBJ/usart2.c)

读完这几处，基本就能弄清楚：

- 遥控数据从哪里进来
- 飞控控制在哪个周期执行
- PID 怎么初始化和保存
- 电机输出怎么计算
- 上位机怎么读写参数

## 11. 当前代码的一些注意点

### 1. 主逻辑在中断里

这个项目最容易忽略的一点是：

- `while(1)` 几乎不跑控制逻辑
- 真正的控制任务在 `TIM3_IRQHandler()` 里

后续加功能时，要特别注意中断执行时间，避免把 5ms 周期拖长。

### 2. 机型开关是编译期开关

在 [USER/precompile.h](USER/precompile.h) 里只能选择一种机型：

- `FOUR_AXIS_UAV`
- `FIXED_WING_AIRCRAFT`
- `BRUSHLESS_FOUR_AXIS_UAV`

切换时要保证只有一个宏有效。

### 3. 源码注释可能有编码差异

工程里部分老文件最初更像 `GBK/ANSI` 风格编码，终端或编辑器如果按错编码打开，中文注释会显示异常。  
如果你在 VS Code 里看到注释乱码，建议尝试切换文件编码后再查看。

### 4. 有些模块是预留或未完全启用的

例如：

- 定高、定点 PID 已预留结构，但当前主流程未真正用起来
- `acc_cal.c`、`gyro_cal.c` 中有校准相关逻辑
- `nav.c` 等模块更像后续扩展入口

因此这个工程更适合看作：

- 一个已经能跑起来的基础飞控框架
- 并且预留了后续扩展空间

## 12. 一句话总结

如果用一句话概括，这个项目就是：

> 一个运行在 `STM32F103C8` 上、使用 `MPU6050 + NRF24L01` 的四轴飞控固件，核心实现了 200Hz 姿态控制、无线遥控、PWM 电机输出、匿名上位机调参和 Flash 参数保存。

## 13. 本次已完成的优化

这一节记录的是已经在代码里落地的优化，不是待办建议。

### 1. 串口接收从“中断里直接处理”改成“中断收包 + 主循环执行”

修改位置：

- [OBJ/usart2.c](OBJ/usart2.c)
- [USER/main.c](USER/main.c)

如何优化：

- `USART2_IRQHandler()` 现在只负责按字节组帧
- 收到完整帧后，仅把数据复制到待处理缓冲区
- 新增 `Usart2Task()`，在主循环中完成命令解析与执行

优化效果：

- 避免在串口中断里做 `delay_ms()`、PID 回传和 Flash 擦写
- 显著减轻高优先级串口中断对飞控控制节拍的干扰

### 2. DMA 发送改成使用静态缓冲区，避免引用栈上的临时数组

修改位置：

- [OBJ/usart2.c](OBJ/usart2.c)

如何优化：

- 新增 `usart2_dma_tx_buf`
- `UsartDMASendData()` 先把待发数据复制到静态缓冲区，再启动 DMA
- 增加 `usart2_dma_busy` 和轮询回收逻辑，避免 DMA 未完成时重复启动

优化效果：

- 消除 DMA 异步发送时访问失效栈内存的风险
- 遥测发送更加稳定

### 3. 阻塞串口发送从“每字节等待 TC”改成“每字节等待 TXE，末尾再等待 TC”

修改位置：

- [OBJ/usart2.c](OBJ/usart2.c)

如何优化：

- `Usart2Send()` 发送每个字节时只等待 `TXE`
- 所有字节写完后再统一等待一次 `TC`
- 若 DMA 正在发送，则先等待 DMA 空闲，避免两种发送方式互相冲突

优化效果：

- 缩短串口阻塞发送时间
- PID 参数回传和校验帧发送效率更高

### 4. 电机混控输出改为有符号中间量，再统一限幅

修改位置：

- [OBJ/controller.h](OBJ/controller.h)
- [OBJ/controller.c](OBJ/controller.c)
- [OBJ/pwm.h](OBJ/pwm.h)
- [OBJ/pwm.c](OBJ/pwm.c)
- [OBJ/imath.h](OBJ/imath.h)
- [OBJ/imath.c](OBJ/imath.c)

如何优化：

- `MotorX.out` 和 `throttle.FINAL_OUT` 改成 `int32_t`
- 新增 `LimitInt32()` 做有符号限幅
- `PwmOut()` 在最终写入 TIM CCR 前，再统一做夹紧和类型转换

优化效果：

- 避免负值先转换成超大无符号数再参与限幅
- 输出行为更符合姿态控制预期

### 5. 姿态 PID 积分清零逻辑做了收口

修改位置：

- [OBJ/controller.c](OBJ/controller.c)

如何优化：

- 提取 `ResetAttitudePidIntegral()`
- 低油门和未解锁两个分支复用同一套清零逻辑

优化效果：

- 降低重复代码
- 后续维护 PID 状态更集中

### 6. 上位机状态遥测从 200Hz 降到 50Hz

修改位置：

- [OBJ/timer.c](OBJ/timer.c)

如何优化：

- 在 `TIM3_IRQHandler()` 中加入 `telemetry_div`
- 每 4 个控制周期才发送一次 `ANOSendStatus()`

优化效果：

- 降低 `TIM3` 中断中的串口与 DMA 开销
- 让 5ms 控制中断更专注于姿态解算和控制输出

### 7. SPI 引脚初始化修正为更合理的主机模式配置

修改位置：

- [OBJ/spi.c](OBJ/spi.c)

如何优化：

- `PA5 / PA7` 作为 `SCK / MOSI` 配置为复用推挽
- `PA6` 作为 `MISO` 配置为浮空输入
- 同时补齐初始化结构体的零初始化

优化效果：

- 更符合 STM32 SPI 主机模式常规配置
- 降低 SPI 读回异常风险

### 8. PWM 初始化参数改为只读常量，并补齐结构体初始化

修改位置：

- [OBJ/pwm.c](OBJ/pwm.c)

如何优化：

- `arrValue / pscValue / ccrValue` 改成 `static const`
- GPIO 和 TIM 初始化结构体统一零初始化

优化效果：

- 避免初始化参数被误修改
- 减少未初始化字段带来的隐患
