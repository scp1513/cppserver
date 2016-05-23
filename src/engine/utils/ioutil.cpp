#include "ioutil.h"
#include <fstream>

bool ioutil::ReadFile(const char * filename, std::vector<char>& out)
{
	std::ifstream in(filename, std::ifstream::in);
	if (!in) return false;

	auto* pbuf = in.rdbuf();
	size_t size = (size_t)pbuf->pubseekoff(0, std::ios::end, std::ios::in);
	pbuf->pubseekpos(0, std::ios::in);

	out.resize(size);
	pbuf->sgetn(&out[0], size);

	return true;
}
