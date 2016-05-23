#ifndef __UTILS_FIXED_STR_HEADER__
#define __UTILS_FIXED_STR_HEADER__

#include <assert.h>
#include <string.h>

namespace utils
{
//#pragma pack(push, 1)
	template <unsigned int LENGHT>
	struct FixedStr
	{
		typedef FixedStr<LENGHT> SelfType;
		typedef char RawType[LENGHT];
		RawType val;

		static const size_t LEN = LENGHT;

		FixedStr() { memset(val, 0, sizeof(val)); }

		const char* c_str() const { return val; }
		operator const RawType&() const { return val; }

		SelfType& operator=(const SelfType& rh)
		{
			memcpy(val, rh.val, sizeof(val));
			val[LENGHT - 1] = '\0';
			return *this;
		}

		SelfType& operator=(const RawType& rh)
		{
			memcpy(val, rh, sizeof(val));
			val[LENGHT - 1] = '\0';
			return *this;
		}

		char& operator[](const int index)
		{
			if (index >= LENGHT)
			{
				assert(false);
				return val[LENGHT - 1];
			}
			return val[index];
		}

		const char& operator[](const int index) const
		{
			if (index >= LENGHT)
			{
				assert(false);
				return val[LENGHT - 1];
			}
			return val[index];
		}
	};

	template <> struct FixedStr<0>;
//#pragma pack(pop)
}

#endif