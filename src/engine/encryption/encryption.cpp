#include "encryption.h"
#include <utils/random.h>

namespace encryption
{

	int CodeV1(uint32 randKey, const char *key, unsigned int keyLen, char *dst, const char *src, unsigned int srclen)
	{
		int32 k = 0;
		int32 intNum = srclen - (srclen % sizeof(uint32));
		int32 i = 0;
		for (; i < (int32)srclen;)
		{
			if (i < intNum)
			{
				(*(uint32 *)&dst[i]) =
					(*(uint32 *)&src[i])
					^
					((*(uint32 *)&key[k]) ^ (randKey + i));
				i += sizeof(uint32);
			}
			else
			{
				(*(char *)&dst[i]) =
					(*(char *)&src[i])
					^
					((*(char *)&key[k]) ^ (randKey + i));
				i += sizeof(char);
			}
			k = i % keyLen;
		}
		return i;
	}

	Version version = Version_Raw;

	void SetEncVersion(Version ver)
	{
		version = ver;
	}

	Version GetEncVersion()
	{
		return version;
	}

	void NewKey(char *key, unsigned int keylen)
	{
		for (unsigned int i = 0; i < keylen; ++i)
		{
			key[i] = utils::Rand32(1, 255);
		}
	}

	int Encode(uint32 randKey, const char *key, unsigned int keyLen, char *dst, const char *src, unsigned int srclen)
	{
		switch (version)
		{
		case Version_Raw:
			break;
		case Version_V1:
			return CodeV1(randKey, key, keyLen, dst, src, srclen);
			break;
		}
		return 0;
	}

	int Decode(uint32 randKey, const char *key, unsigned int keyLen, char *dst, const char *src, unsigned int srclen)
	{
		switch (version)
		{
		case Version_Raw:
			break;
		case Version_V1:
			return CodeV1(randKey, key, keyLen, dst, src, srclen);
			break;
		}
		return 0;
	}

}