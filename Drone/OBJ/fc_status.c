/*
 * ????????????????????????????????????
 */
#include "fc_status.h"
#include "parse_packet.h"
#include "precompile.h"

extern PlaneData plane;

// 根据摇杆动作和持续时间判断上锁/解锁。
// 这里使用计数方式做防误触，必须连续满足条件多个控制周期才会切换状态。
void PlaneLockStatus(void)
{
	  // 解锁动作：油门低位，同时摇杆打到指定角落并保持。
		#if FOUR_AXIS_UAV
    if(plane.throttle <= 10 && plane.yaw <= -25 && plane.pit >= 25 && plane.rol <= -25){
        plane.unlockCount++;
		}
    if(plane.throttle <= 10 && plane.yaw <= -25 && plane.pit >= 25 && plane.rol <= -25 && plane.unlockCount >= 600){
        plane.unlockCount = 0;
        plane.lock = UNLOCK;		//解锁标志置位，指示灯使用该标志                             
    }
		#elif	FIXED_WING_AIRCRAFT
		if(plane.throttle <= 10 && plane.yaw <= -25){
        plane.unlockCount++;
		}
    if(plane.throttle <= 10 && plane.yaw <= -25 && plane.unlockCount >= 600){
        plane.unlockCount = 0;
        plane.lock = UNLOCK;		//解锁标志置位，指示灯使用该标志                             
    }
		#elif BRUSHLESS_FOUR_AXIS_UAV
		if(plane.throttle <= 10 && plane.yaw <= -25 && plane.pit >= 25 && plane.rol <= -25){
        plane.unlockCount++;
		}
    if(plane.throttle <= 10 && plane.yaw <= -25 && plane.pit >= 25 && plane.rol <= -25 && plane.unlockCount >= 600){
        plane.unlockCount = 0;
        plane.lock = UNLOCK;		//解锁标志置位，指示灯使用该标志                             
    }
		#endif
		
    // 上锁动作：
    // 1. 油门低位保持很久；
    // 2. 或油门低位加偏航打到另一侧保持一段时间。
    if(plane.throttle <= 10){
        plane.lockCount++;
		}
    if((plane.throttle <= 10 && plane.lockCount >= 4000) || (plane.throttle <= 10 && plane.yaw >= 25 && plane.lockCount >= 600)){
        plane.lockCount = 0;
        plane.lock = LOCK;		//上锁标志置位，指示灯使用该标志                               
    }
		
		if(plane.throttle > 10){
			plane.lockCount = 0;		// 油门抬起后取消上锁计时
		}
}



uint32_t  chip_id[3] = {0};  

// 读取 STM32 的唯一芯片 ID，便于做设备标识或后续绑定扩展。
void get_chip_id(void)
{
    chip_id[0] = *(__IO u32 *)(0X1FFFF7F0); // 高 32 位
    chip_id[1] = *(__IO u32 *)(0X1FFFF7EC); // 中 32 位
    chip_id[2] = *(__IO u32 *)(0X1FFFF7E8); // 低 32 位
}








