/*
 * 飞控控制器模块。
 *
 * 该文件位于 TIM3 控制周期的后半段，主要负责控制模式选择、串级 PID 运算、
 * 电机混控、异常姿态保护以及最终 PWM 输出。
 */
#include "controller.h"
#include "pid.h"
#include "imu.h"
#include "nrf24l01.h"
#include "pwm.h"
#include "mpu6050.h"
#include "parse_packet.h"
#include "fc_status.h"
#include "precompile.h"

/*
 * 控制器数据主线：
 * 遥控器解析结果 plane.rol/pit/yaw/throttle
 *      + 姿态解算结果 att.rol/pit/yaw
 *      + 陀螺仪角速度 Mpu.deg_s
 *      -> 姿态角外环 PID
 *      -> 角速度内环 PID
 *      -> 电机混控
 *      -> PwmOut() 输出到电机。
 */

_CONTROLLER_CNT controller_cnt = {0};
Throttle throttle = {0};
_CONTROLLER_MODE _control = {0};

/* 油门超过该值后，认为进入可飞行输出区间，才叠加姿态修正量。 */
#define THROTTLE_FLYING  10

/*
 * 起飞辅助参数。
 * 满足解锁、信号正常、油门低、三轴摇杆基本回中，并按下指定按键时，
 * 自动缓慢抬升油门，帮助飞机平顺离地。
 */
#define TAKEOFF_ASSIST_KEY_R            0xC8
#define TAKEOFF_ASSIST_CANCEL_KEY_L     0xE1
#define TAKEOFF_ASSIST_TRIGGER_THR      60
#define TAKEOFF_ASSIST_START_THR        80
#define TAKEOFF_ASSIST_TARGET_THR       280
#define TAKEOFF_ASSIST_RAMP_STEP        1
#define TAKEOFF_ASSIST_RELEASE_STEP     2
#define TAKEOFF_ASSIST_ARM_STICK        8.0f
#define TAKEOFF_ASSIST_ABORT_STICK      20.0f
#define TAKEOFF_ASSIST_HANDOVER_MARGIN  20
#define TAKEOFF_ASSIST_PACKET_LOSS_MAX  2

_OUT_Motor Motor1 = {0};
_OUT_Motor Motor2 = {0};
_OUT_Motor Motor3 = {0};
_OUT_Motor Motor4 = {0};

extern AllPid allPid;
extern PlaneData plane;
extern Mpu6050_Data Mpu;

/* 起飞辅助内部状态。 */
typedef struct
{
	/* active = 1 表示起飞辅助正在接管油门抬升。 */
	uint8_t active;

	/* 当前起飞辅助给出的油门值。 */
	uint16_t throttle_out;
}_TAKEOFF_ASSIST;

static _TAKEOFF_ASSIST takeoffAssist = {0};

/* 关闭起飞辅助，并清空辅助油门输出。 */
static void ResetTakeoffAssist(void)
{
	takeoffAssist.active = 0;
	takeoffAssist.throttle_out = 0;
}

/* 判断某个摇杆通道是否处在以 0 为中心的安全窗口内。 */
static uint8_t IsStickInsideWindow(float value, float limit)
{
	return (value >= -limit && value <= limit);
}

/*
 * 起飞辅助启动前的摇杆安全检查。
 * 要求油门较低，并且 pitch/roll/yaw 基本回中，避免飞手正在主动控姿时误触发。
 */
static uint8_t IsTakeoffAssistArmReady(void)
{
	return (plane.throttle <= TAKEOFF_ASSIST_TRIGGER_THR
		&& IsStickInsideWindow(plane.pit, TAKEOFF_ASSIST_ARM_STICK)
		&& IsStickInsideWindow(plane.rol, TAKEOFF_ASSIST_ARM_STICK)
		&& IsStickInsideWindow(plane.yaw, TAKEOFF_ASSIST_ARM_STICK));
}

/*
 * 判断飞手是否明显接管。
 * 任意姿态通道偏离中心较多时，起飞辅助会逐步退出，把控制权交还给飞手。
 */
static uint8_t IsTakeoffAssistPilotOverride(void)
{
	return (!IsStickInsideWindow(plane.pit, TAKEOFF_ASSIST_ABORT_STICK)
		|| !IsStickInsideWindow(plane.rol, TAKEOFF_ASSIST_ABORT_STICK)
		|| !IsStickInsideWindow(plane.yaw, TAKEOFF_ASSIST_ABORT_STICK));
}

/*
 * 起飞辅助油门更新。
 * 该函数不直接输出 PWM，只在安全条件满足时提高 throttle.FINAL_OUT。
 */
static void TakeoffAssistUpdate(void)
{
	/* 未解锁、信号异常、丢包过多或按下取消键时，立即关闭起飞辅助。 */
	if(plane.lock != UNLOCK || plane.signal != SIGNAL_NORMAL || plane.signalLostCount > TAKEOFF_ASSIST_PACKET_LOSS_MAX || plane.key_l == TAKEOFF_ASSIST_CANCEL_KEY_L){
		ResetTakeoffAssist();
		return;
	}

	/* 未激活时，只有按下起飞辅助键且摇杆处于安全窗口内，才进入辅助状态。 */
	if(takeoffAssist.active == 0){
		if(plane.key_r != TAKEOFF_ASSIST_KEY_R || IsTakeoffAssistArmReady() == 0){
			return;
		}

		takeoffAssist.active = 1;
		takeoffAssist.throttle_out = plane.throttle;
		if(takeoffAssist.throttle_out < TAKEOFF_ASSIST_START_THR){
			takeoffAssist.throttle_out = TAKEOFF_ASSIST_START_THR;
		}
	}

	/*
	 * 飞手油门已经接近辅助油门时，说明飞手可以自然接管。
	 * 此时退出辅助，避免继续覆盖飞手的油门输入。
	 */
	if(plane.throttle + TAKEOFF_ASSIST_HANDOVER_MARGIN >= takeoffAssist.throttle_out){
		ResetTakeoffAssist();
		return;
	}

	/*
	 * 保持按键且没有明显打杆时，辅助油门缓慢上升。
	 * 松开按键或飞手明显打杆时，辅助油门逐步下降并最终退出。
	 */
	if(plane.key_r == TAKEOFF_ASSIST_KEY_R && IsTakeoffAssistPilotOverride() == 0){
		if(takeoffAssist.throttle_out < TAKEOFF_ASSIST_TARGET_THR){
			takeoffAssist.throttle_out += TAKEOFF_ASSIST_RAMP_STEP;
		}
	}else{
		if(takeoffAssist.throttle_out > plane.throttle + TAKEOFF_ASSIST_RELEASE_STEP){
			takeoffAssist.throttle_out -= TAKEOFF_ASSIST_RELEASE_STEP;
		}else{
			ResetTakeoffAssist();
			return;
		}
	}

	/* 起飞辅助只抬高最终油门，不压低飞手给出的更高油门。 */
	if(takeoffAssist.throttle_out > throttle.FINAL_OUT){
		throttle.FINAL_OUT = takeoffAssist.throttle_out;
	}
}

// 停机、低油门或未解锁时清空双环 PID 积分，防止重新起飞时残留控制量。
static void ResetAttitudePidIntegral(void)
{
	ClearIntegral(&allPid.pitAngle);
	ClearIntegral(&allPid.pitGyro);
	ClearIntegral(&allPid.rolAngle);
	ClearIntegral(&allPid.rolGyro);
	ClearIntegral(&allPid.yawAngle);
	ClearIntegral(&allPid.yawGyro);
}

// 姿态角外环：
// roll/pitch 直接按目标姿态闭环；
// yaw 在起飞初期先等待姿态稳定，之后采用“回中锁航向、打舵给角速度”的控制策略。
void AngleController(void)
{
	static uint16_t yaw_init_cnt = 0;

	/*
	 * Roll 姿态角外环：
	 * expect 在 ControModel() 中由 plane.rol 设置；
	 * feedback 使用 IMU 解算出的当前横滚角 att.rol；
	 * PID 输出 allPid.rolAngle.out 会作为 roll 角速度内环的期望值。
	 */
	allPid.rolAngle.feedback = att.rol;
	PidController(&allPid.rolAngle);

	/*
	 * Pitch 姿态角外环：
	 * expect 在 ControModel() 中由 plane.pit 设置；
	 * feedback 使用当前俯仰角 att.pit；
	 * PID 输出 allPid.pitAngle.out 会作为 pitch 角速度内环的期望值。
	 */
	allPid.pitAngle.feedback = att.pit;
	PidController(&allPid.pitAngle);

	/*
	 * Yaw 在前 300 个控制周期不启用外环。
	 * 这样可以给刚启动/刚起飞阶段留一点姿态稳定时间，避免航向初值不稳造成误修正。
	 */
	if(yaw_init_cnt < 300){
		yaw_init_cnt++;
	}else{
		/*
		 * yaw 摇杆回中时锁定航向：
		 * 第一次回中时把当前 att.yaw 记录为期望航向；
		 * 后续通过 yawAngle PID 维持这个航向。
		 */
		if(plane.yaw == 0){
			if(allPid.yawAngle.expect == 0){
				allPid.yawAngle.expect = att.yaw;
			}
			allPid.yawAngle.feedback = att.yaw;
			PidController(&allPid.yawAngle);
			allPid.yawGyro.expect = allPid.yawAngle.out;
		}else{
			/*
			 * yaw 摇杆有输入时，不再锁航向，而是把摇杆量映射成期望偏航角速度。
			 * 这里的 * 5 是比例缩放，实际手感由遥控解析范围和 PID 参数共同决定。
			 */
			allPid.yawAngle.expect = 0;
			allPid.yawGyro.expect = plane.yaw * 5;
		}
	}
}

// 角速度内环：
// pitch/roll 的期望值来自姿态角外环，yaw 的期望值由上面的偏航策略决定。
void GyroController(void)
{
	/*
	 * Pitch 角速度内环：
	 * 期望角速度来自 pitch 姿态角外环输出；
	 * 反馈值来自陀螺仪 X 轴角速度 Mpu.deg_s.x。
	 */
	allPid.pitGyro.expect = allPid.pitAngle.out;
	allPid.pitGyro.feedback = Mpu.deg_s.x;
	PidController(&allPid.pitGyro);
	/*
	 * Roll 角速度内环：
	 * 期望角速度来自 roll 姿态角外环输出；
	 * 反馈值来自陀螺仪 Y 轴角速度 Mpu.deg_s.y。
	 */
	allPid.rolGyro.expect = allPid.rolAngle.out;
	allPid.rolGyro.feedback = Mpu.deg_s.y;
	PidController(&allPid.rolGyro);
	/*
	 * Yaw 角速度内环：
	 * 期望值由 AngleController() 中的偏航策略设置；
	 * 反馈值来自陀螺仪 Z 轴角速度 Mpu.deg_s.z。
	 */
	allPid.yawGyro.feedback = Mpu.deg_s.z;
	PidController(&allPid.yawGyro);
}


uint8_t high_mark_flag = 0;
uint8_t fix_mark_flag = 0;

// 控制模式选择层。
// 当前工程只启用了基础姿态模式：遥控器直接给出目标姿态角和油门。
void ControModel(void)
{
	/*
	 * 当前工程只启用 mode = 1：基础姿态控制模式。
	 * 如果后续加入定高、定点、光流等模式，可以在这里根据遥控开关切换 _control.mode。
	 */
	_control.mode = 1;

	/*
	 * 遥控器解析后的 roll/pitch 直接作为姿态角外环目标。
	 * 油门先写入 FINAL_OUT，随后 TakeoffAssistUpdate() 可能在安全条件下抬高该值。
	 */
	allPid.rolAngle.expect = plane.rol;
	allPid.pitAngle.expect = plane.pit;
	throttle.FINAL_OUT = plane.throttle;
	TakeoffAssistUpdate();
}

// 根据当前模式调度控制器。
void _controller_perform(void)
{
	/*
	 * 控制器调度层。
	 * mode = 1 时执行串级 PID：先姿态角外环，再角速度内环。
	 */
	switch(_control.mode)
	{
		case 1:
			AngleController();
			GyroController();
			break;
		default:
			break;
	}
}

// 大姿态保护：机体翻转过大时直接上锁，避免电机继续输出。
void UnusualProtect(void)
{
	/*
	 * 大姿态保护。
	 * roll 或 pitch 超过 +/-80 度时，认为机体已经严重倾斜或翻倒，
	 * 直接切换到 LOCK，后面的 ControllerOutput() 会进入停机输出分支。
	 */
	if(att.rol <= -80 || att.rol >= 80 || att.pit <= -80 || att.pit >= 80){
		plane.lock = LOCK;
	}
}

// 电机混控与最终输出：
// 已解锁且油门足够时叠加三轴控制量；低油门或未解锁时进入安全输出。
void ControllerOutput(void)
{
	/*
	 * 电机输出前先检查大姿态保护。
	 * UnusualProtect() 可能会把 plane.lock 改成 LOCK，
	 * 因此异常姿态会在本次输出周期内立即阻断电机输出。
	 */
#if FOUR_AXIS_UAV
	UnusualProtect();
#elif FIXED_WING_AIRCRAFT
	// not used
#elif BRUSHLESS_FOUR_AXIS_UAV
	UnusualProtect();
#endif

	if(plane.lock == UNLOCK)
	{
		/*
		 * 已解锁且油门超过起飞阈值时，才叠加姿态控制量。
		 * 低油门时不叠加 PID 修正，可以避免飞机在地面上因姿态误差产生剧烈电机差动。
		 */
		if(throttle.FINAL_OUT > THROTTLE_FLYING)
		{
#if FOUR_AXIS_UAV
			// X 型四轴混控。
			// 油门作为公共基础量，pitch/roll/yaw 内环输出按不同正负号叠加，形成三轴修正力矩。
			Motor1.out = (int32_t)(throttle.FINAL_OUT + allPid.pitGyro.out - allPid.rolGyro.out - allPid.yawGyro.out);
			Motor2.out = (int32_t)(throttle.FINAL_OUT + allPid.pitGyro.out + allPid.rolGyro.out + allPid.yawGyro.out);
			Motor3.out = (int32_t)(throttle.FINAL_OUT - allPid.pitGyro.out + allPid.rolGyro.out - allPid.yawGyro.out);
			Motor4.out = (int32_t)(throttle.FINAL_OUT - allPid.pitGyro.out - allPid.rolGyro.out + allPid.yawGyro.out);
#elif FIXED_WING_AIRCRAFT
			// 固定翼分支保留了另一套混控形式；当前工程中该分支标记为 not used。
			Motor1.out = (int32_t)(throttle.FINAL_OUT + 2 * allPid.yawGyro.out + allPid.pitGyro.out);
			Motor2.out = (int32_t)(throttle.FINAL_OUT - 2 * allPid.yawGyro.out + allPid.pitGyro.out);
			Motor3.out = (int32_t)(throttle.FINAL_OUT - 2 * allPid.yawGyro.out + allPid.pitGyro.out);
			Motor4.out = (int32_t)(throttle.FINAL_OUT + 2 * allPid.yawGyro.out + allPid.pitGyro.out);
#elif BRUSHLESS_FOUR_AXIS_UAV
			// 无刷四轴使用同样的 X 型混控，具体 PWM 限幅/输出由 PwmOut() 或更底层驱动处理。
			Motor1.out = (int32_t)(throttle.FINAL_OUT + allPid.pitGyro.out - allPid.rolGyro.out - allPid.yawGyro.out);
			Motor2.out = (int32_t)(throttle.FINAL_OUT + allPid.pitGyro.out + allPid.rolGyro.out + allPid.yawGyro.out);
			Motor3.out = (int32_t)(throttle.FINAL_OUT - allPid.pitGyro.out + allPid.rolGyro.out - allPid.yawGyro.out);
			Motor4.out = (int32_t)(throttle.FINAL_OUT - allPid.pitGyro.out - allPid.rolGyro.out + allPid.yawGyro.out);
#endif
		}
		else
		{
			// 低油门时不继续叠加姿态修正，避免电机在地面上剧烈修正。
			Motor1.out = throttle.FINAL_OUT;
			Motor2.out = throttle.FINAL_OUT;
			Motor3.out = throttle.FINAL_OUT;
			Motor4.out = throttle.FINAL_OUT;
			ResetAttitudePidIntegral();
		}
	}
	else
	{
		/*
		 * 未解锁或保护逻辑触发上锁时，四个电机输出全部清零。
		 * 同时清空起飞辅助状态和 PID 积分，避免下次解锁/起飞时残留控制量造成冲击。
		 */
		Motor1.out = 0;
		Motor2.out = 0;
		Motor3.out = 0;
		Motor4.out = 0;
		ResetTakeoffAssist();
		ResetAttitudePidIntegral();
	}

	/* 将四路最终输出交给 PWM 驱动层。 */
	PwmOut(Motor1.out,Motor2.out,Motor3.out,Motor4.out);
}
