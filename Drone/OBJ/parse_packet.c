/*
 * 模块说明：无线控制包解析实现，同时负责组织 ACK 回传包。
 */
#include "parse_packet.h"
#include "nrf24l01.h"
#include "imath.h"
#include "controller.h"
#include "pair_freq.h"
#include "precompile.h"

/* 接收到的遥控数据包缓存。 */
u8 Rx_packet[RX_PLOAD_WIDTH] = {0};

/* 通过 ACK 负载回传给遥控器的数据包缓存。 */
u8 txPacket[TX_PLOAD_WIDTH] = {0};

/* 飞控当前运行状态。 */
PlaneData plane = {0};

/*
 * 解析遥控器发来的控制包。
 *
 * 包尾固定为 0x8B。
 * 收到合法数据后会刷新油门、三轴控制量和功能键状态，
 * 同时根据不同机型做不同的量纲处理。
 */
/*
 * 解析遥控器发来的控制包。
 *
 * 这段逻辑的核心目标是：
 * 1. 从 nRF24L01 中取出最新一帧遥控数据。
 * 2. 检查这帧数据是不是合法帧。
 * 3. 把数据包里的各个字段翻译成 plane 结构体里的控制量。
 * 4. 如果长时间收不到新包，则把链路状态判定为失联，并做保护处理。
 * 当前协议里，包尾固定使用 0x8B 作为简单的合法帧标志。
 */
void nrf_parse_packet(void)
{
    /*
     * NRF24L01_RxPacket() 返回 0 表示“成功收到一帧新数据”；
     * 返回非 0 表示“当前没有读到新包”。
     */
    if (NRF24L01_RxPacket(Rx_packet) == 0)
    {
        /*
         * 先检查包尾。
         * 如果最后一个字节不是约定好的 0x8B，就直接丢弃这帧，
         * 避免把错误数据当成遥控指令解析。
         */
        if (Rx_packet[RX_PLOAD_WIDTH - 1] != 0x8B)
        {
            return;
        }
        /*
         * 只要成功收到一帧合法数据，就认为无线链路处于正常状态：
         * 1. 丢包计数清零
         * 2. 信号状态改为正常
         */
        plane.signalLostCount = 0;
        plane.signal = SIGNAL_NORMAL;
        /*
         * 油门由两个字节组成：
         * Rx_packet[1] 是低字节
         * Rx_packet[2] 是高字节
         * 组合方式就是：高字节左移 8 位，再与低字节拼起来。
         */
        plane.throttle = (Rx_packet[2] << 8) | Rx_packet[1];
        #if FOUR_AXIS_UAV
            /*
            * 这里先把无符号字节强制解释成有符号 int8_t，
            * 再减去协议中心值 50，最后根据机体坐标方向决定是否取负号。
            * 这样做完后，plane.pit / rol / yaw 就变成以 0 为中点的控制量。
            */
            plane.pit = -(float)(((int8_t)Rx_packet[3]) - 50);
            plane.rol = -(float)(((int8_t)Rx_packet[4]) - 50);
            plane.yaw =  (float)(((int8_t)Rx_packet[5]) - 50);
            /* 有刷四轴把油门限制在 0~950，防止遥控输入超范围。 */
            plane.throttle = ThrottleLimit(plane.throttle, 0, 950);
            /* 功能按键状态直接从协议约定的位置取出。 */
            plane.key_l = Rx_packet[6];
            plane.key_r = Rx_packet[7];
        #elif FIXED_WING_AIRCRAFT
            plane.pit = -(float)(((int8_t)Rx_packet[3]) - 50);
            plane.yaw =  (float)(((int8_t)Rx_packet[4]) - 50);
            plane.rol = -(float)(((int8_t)Rx_packet[5]) - 50);
            /* 固定翼同样做 0~950 的限幅。 */
            plane.throttle = ThrottleLimit(plane.throttle, 0, 950);
            /* 功能按键状态直接从协议约定的位置取出。 */
            plane.key_l = Rx_packet[6];
            plane.key_r = Rx_packet[7];
        #elif BRUSHLESS_FOUR_AXIS_UAV
            plane.pit = -(float)(((Rx_packet[4] << 8) | Rx_packet[3]) - 200);
            plane.rol = -(float)(((Rx_packet[6] << 8) | Rx_packet[5]) - 200);
            plane.yaw = -(float)(((Rx_packet[8] << 8) | Rx_packet[7]) - 200);
            /* 无刷四轴的油门量程更大，因此限制在 0~10000。 */
            plane.throttle = ThrottleLimit(plane.throttle, 0, 10000);
            /* 当前无刷协议中的按键位置尚未确认，先清零避免误用旧值。 */
            plane.key_l = 0;
            plane.key_r = 0;
        #endif
        /*
            * 对输入做“回中死区”处理。
            * 作用：
            * 1. 当摇杆接近中位时，直接把微小抖动压成 0
            * 2. 避免因为遥控器采样误差或手抖，让飞机一直收到很小的控制量
        */
        plane.throttle = direction_to_zero(plane.throttle, 0, 5);
        plane.pit = direction_to_zero(plane.pit, -5, 5);
        plane.rol = direction_to_zero(plane.rol, -5, 5);
        plane.yaw = direction_to_zero(plane.yaw, -5, 5);
    }
    else
    {
        /*
         * 如果这次没有收到新包，也不立即判定失联，
         * 因为无线通信里偶尔丢一两包是正常现象。
         * 所以这里先做“连续丢包计数”：
         * 只要计数还没到阈值，就继续认为链路暂时正常。
         */
        if (plane.signalLostCount < 200)
        {
            plane.signalLostCount++;
            plane.signal = SIGNAL_NORMAL;
        }
        /*
         * 当连续丢包计数达到 200 时，才正式认为“信号已丢失”。
         * 这相当于做了一个简单的软件防抖，避免误判。
         */
        if (plane.signalLostCount == 200)
        {
            plane.signal = SIGNAL_LOST;
            /*
             * 失联后不要立刻把油门清零，而是缓慢收油。
             * 这样做通常比瞬间断动力更平滑，可以降低失控风险。
             */
            if (plane.throttle > 10)
            {
                plane.throttle -= 1;
            }
        }
    }
}

/*
if 接收到指令包：
    if 指令包不合规：
        return;
    更新油门
    #if 四轴无人机：
        更新俯仰角pti
        更新横滚角roll
        更新偏航角yaw
        更新油门throttle和左右按键
    #elif 固定翼：
         更新俯仰角pti
        更新横滚角roll
        更新偏航角yaw
        更新油门throttle和左右按键
    #elif 无刷四轴：
         更新俯仰角pti
        更新横滚角roll
        更新偏航角yaw
        更新油门throttle和左右按键
    #endif
    回中死区处理
else 未接收：
    if 失联计数<200:
        继续计数，状态还是显示正常
    if 失联计数==200：
        更新状态失联
        if 油门>10:
            缓慢降低油门
*/
/*
油门 throttle：控制升力大小，主要影响 z 方向
偏航 yaw：控制绕 z 轴旋转
俯仰 pitch：控制前后倾斜，进而影响前后运动
横滚 roll：控制左右倾斜，进而影响左右运动
*/

/*
 * 组织 ACK 回传包。
 *
 * 当前回传内容包括：
 * 1. 锁定状态。
 * 2. 电量状态。
 * 3. 当前电池电压，放大 100 倍后用两个字节回传。
 */
void NrfACKPacket(void)
{
    uint16_t temp = 0;
    txPacket[0] = 0xAA;  //包头
    txPacket[1] = plane.lock;
    txPacket[2] = plane.power;
    temp = (uint16_t)(plane.voltage * 100);  //放大了，就要占两个字节了
    txPacket[3] = *((u8 *)&temp);   //电压
    txPacket[4] = *((u8 *)&temp + 1);
    txPacket[TX_PLOAD_WIDTH - 1] = 0xAC; //包尾
    SPI_Write_Buf(W_ACK_PLOAD, txPacket, TX_PLOAD_WIDTH);
}

/*
把无人机当前状态打包成 txPacket，
写入 nRF24L01 的 ACK 负载缓冲区，
这样下次遥控器给我发包时，
我就在 ACK 应答里把这些状态一起回给它。
*/

/*
*((u8 *)&temp)
可以分成 3 步看：
&temp
取 temp 的地址
(u8 *)&temp
把“temp 的地址”强制看成 u8 * 类型
也就是：把它当成“指向 1 字节数据的指针”
*((u8 *)&temp)
对这个指针解引用，取出 temp 在内存里的第 1 个字节
如果写得更容易懂一点，也可以写成位运算形式：
txPacket[3] = temp & 0xFF;         // 低字节
txPacket[4] = (temp >> 8) & 0xFF;  // 高字节
这个和指针拆法本质上是一个意思，而且对初学者通常更直观。
*/