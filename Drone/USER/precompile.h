#ifndef _precompile_h
#define	_precompile_h
#include "stm32f10x.h"

#define FOUR_AXIS_UAV              0   // 四轴空心杯无人机
#define FIXED_WING_AIRCRAFT        0   // 固定翼手抛机
#define BRUSHLESS_FOUR_AXIS_UAV    1   // 无刷四轴无人机

#if FOUR_AXIS_UAV
#elif FIXED_WING_AIRCRAFT
#elif BRUSHLESS_FOUR_AXIS_UAV
#endif

#endif

