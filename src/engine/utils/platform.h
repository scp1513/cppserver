#pragma once

#if defined(linux) || defined(__linux) || defined(__linux__)
# define PLATFORM_LINUX
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
# define PLATFORM_WIN32
#else
# error "common unavailable on this platform"
#endif
