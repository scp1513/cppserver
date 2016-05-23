#include <utils/system.h>
#include <utils/platform.h>

#ifdef PLATFORM_WIN32
#include <windows.h>
#include <process.h>
#elif defined(PLATFORM_LINUX)
#include <unistd.h>
#endif

namespace utils
{
	// 获取cpu数量
	uint32 GetCPUNum()
	{
		uint32 count = 1;
#if defined(PLATFORM_WIN32)
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		count = si.dwNumberOfProcessors;
#elif defined(PLATFORM_LINUX)
		count = sysconf(_SC_NPROCESSORS_CONF);
#else
		static_assert(false, "");
#endif
		return count;
	}

	int GetPid()
	{
#if defined(PLATFORM_WIN32)
		return (int)_getpid();
#else
		return (int)getpid();
#endif
	}

}
