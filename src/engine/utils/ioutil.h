#ifndef __UTILS_IOUTIL_HEADER__
#define __UTILS_IOUTIL_HEADER__

#include <vector>

class ioutil
{
	ioutil() = delete;

public:
	static bool ReadFile(const char* filename, std::vector<char>& out);
};

#endif