#ifndef __UTILS_RANDOM_HEADER__
#define __UTILS_RANDOM_HEADER__

#include <utils/typedef.h>

namespace utils
{
	// [0, max)
	uint32 RandMax32(uint32 max);

	// [min, max)
	uint32 Rand32(uint32 min, uint32 max);
}

#endif
