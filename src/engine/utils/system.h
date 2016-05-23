#ifndef __UTILS_SYSTEM_INFO_HEADER__
#define __UTILS_SYSTEM_INFO_HEADER__

#include <utils/typedef.h>

namespace utils
{
	// 获取cpu数量
	uint32 GetCPUNum();

	// 获取当前进程id
	int GetPid();
}

#endif