#include "random.h"
#include <random>
#include <time.h>

namespace utils
{
	uint32 RandMax32(uint32 max)
	{
		if (max <= 1) return 0;

		static std::mt19937 gen((unsigned int)time(nullptr));
		std::uniform_int_distribution<uint32> dist(0, max - 1);

		return dist(gen);
	}

	uint32 Rand32(uint32 min, uint32 max)
	{
		if (max < min) return 0;
		if (max == min) return min;

		return RandMax32(max - min) + min;
	}
}