#ifndef __ENCRYPT_HEADER__
#define __ENCRYPT_HEADER__

#include <utils/typedef.h>

namespace encryption {
	enum Version {
		Version_Raw,
		Version_V1,
	};

	void SetEncVersion(Version ver);
	Version GetEncVersion();

	void NewKey(char *key, unsigned int keylen);

	int Encode(uint32 randKey, const char *key, unsigned int keyLen, char *dst, const char *src, unsigned int srclen);
	int Decode(uint32 randKey, const char *key, unsigned int keyLen, char *dst, const char *src, unsigned int srclen);
}

#endif