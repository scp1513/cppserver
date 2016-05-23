#ifndef __UTILS_MD5_HEADER__
#define __UTILS_MD5_HEADER__

#include <string>

namespace utils
{
	std::string MD5(const void* data, unsigned int len);
	std::string MD5File(const std::string& filename);
}

#endif