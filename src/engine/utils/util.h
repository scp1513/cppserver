#pragma once

#include "typedef.h"
#include "platform.h"
#include <string>
#include <sstream>
#include <vector>
#include <stdio.h>

#ifdef ARRSIZE
# undef ARRSIZE
#endif
#define ARRSIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#ifdef EC_SPRINTF
# undef EC_SPRINTF
#endif
#ifdef EC_VSPRINTF
# undef EC_VSPRINTF
#endif

#if defined(PLATFORM_WIN32)

# define EC_SPRINTF(buf, size, fmt, ...)\
	_snprintf_s(buf, size, _TRUNCATE, fmt, ##__VA_ARGS__)
# define EC_VSPRINTF(buf, size, fmt, ...)\
	_vsnprintf_s(buf, size, _TRUNCATE, fmt, ##__VA_ARGS__)

#elif defined(PLATFORM_LINUX)

# define EC_SPRINTF(buf, size, fmt, ...)\
	sprintf(buf, fmt, ##__VA_ARGS__)
# define EC_VSPRINTF(buf, size, fmt, ...)\
	vsprintf(buf, fmt, ##__VA_ARGS__)

#endif

#if defined(PLATFORM_WIN32)
# define ATOL _atoi64
#elif defined(PLATFORM_LINUX)
# define ATOL atol
#endif

#ifndef __max
#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef __min
#define __min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

// 64位操作
#define MAKE_PAIR64(l, h)  uint64(uint32(l) | (uint64(h) << 32))
#define PAIR64_HIPART(x)   (uint32)((uint64(x) >> 32) & 0x00000000FFFFFFFFLL)
#define PAIR64_LOPART(x)   (uint32)(uint64(x)   	  & 0x00000000FFFFFFFFLL)
// 32位操作
#define MAKE_PAIR32(l, h)  uint32(uint16(l) | (uint32(h) << 16))
#define PAIR32_HIPART(x)   (uint16)((uint32(x) >> 16) & 0x0000FFFF)
#define PAIR32_LOPART(x)   (uint16)(uint32(x)   	  & 0x0000FFFF)


#ifdef EC_DEBUG
# undef EC_DEBUG
#endif
#ifdef EC_DEBUGF
# undef EC_DEBUGF
#endif
#ifdef EC_DEBUG_IF
# undef EC_DEBUG_IF
#endif
#ifdef EC_DEBUGF_IF
# undef EC_DEBUGF_IF
#endif

#ifdef _DEBUG
# define EC_DEBUG(m__str__)                                                                         \
	do {                                                                                            \
		fprintf(stderr, "EC_DEBUG> %s:%d> %s\n", __FILE__, __LINE__, m__str__);                     \
	} while (false)
#else
# define EC_DEBUG(m__str__) 
#endif

#ifdef _DEBUG
# define EC_DEBUGF(m__fmt__, ...)                                                                   \
	do {                                                                                            \
		fprintf(stderr, "EC_DEBUG> %s:%d> " m__fmt__ "\n", __FILE__, __LINE__, ##__VA_ARGS__);      \
	} while (false)
#else
# define EC_DEBUGF(m__fmt__, ...)
#endif

#ifdef _DEBUG
# define EC_DEBUG_IF(m__conndition__, m__str__)                                                     \
	do {                                                                                            \
		if (m__conndition__)                                                                        \
		{                                                                                           \
			fprintf(stderr, "EC_DEBUG> %s:%d> %s\n", __FILE__, __LINE__, ##m__str__);               \
		}                                                                                           \
	} while (false)
#else
# define EC_DEBUG_IF(m__conndition__, m__fmt__, ...)
#endif

#ifdef _DEBUG
# define EC_DEBUGF_IF(m__conndition__, m__fmt__, ...)                                               \
	do {                                                                                            \
		if (m__conndition__)                                                                        \
		{                                                                                           \
			fprintf(stderr, "EC_DEBUG> %s:%d> " m__fmt__ "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
		}                                                                                           \
	} while (false)
#else
# define EC_DEBUGF_IF(m__conndition__, m__fmt__, ...)
#endif



#define VOID_RETURN

#define Must(condition, ret)    \
	do {                        \
		if (!(condition))       \
		{                       \
			return ret;         \
		}                       \
	} while (false)

namespace utils
{
	template<typename T1, typename T2>
	void InsertFlag(T1& val, const T2& flag)
	{
		val |= flag;
	}

	template<typename T1, typename T2>
	void RemoveFlag(T1& val, const T2& flag)
	{
		val &= ~flag;
	}

	template<typename T1, typename T2>
	bool HasFlag(T1& val, const T2& flag)
	{
		return (val & flag) != 0 ? true : false;
	}

	template<typename T1, typename T2>
	T1 MakeFlag(const T2& pos)
	{
		return static_cast<T1>(0x01u) << (pos - 1);
	}

	template<typename T>
	int GetBitCount(T bits)
	{
		unsigned long long v = bits;
		int c = 0;
		while (v)
		{
			v &= v - 1;
			c++;
		}
		return c;
	}

	// ip convert
	inline uint32 IP2Int(const std::string& str_ip)
	{
		int a[4];
		std::string ip = str_ip;
		std::string strTemp;
		size_t pos;
		size_t i = 3;

		do
		{
			pos = ip.find(".");
			if (pos != std::string::npos)
			{
				strTemp = ip.substr(0, pos);
				a[i] = atoi(strTemp.c_str());
				i--;
				ip.erase(0, pos + 1);
			}
			else
			{
				strTemp = ip;
				a[i] = atoi(strTemp.c_str());
				break;
			}
		} while (1);

		return (a[3] << 24) + (a[2] << 16) + (a[1] << 8) + a[0];
	}

	inline std::string IP2String(unsigned int ip_value)
	{
		std::stringstream str;
		str << ((ip_value & 0xff000000) >> 24) << "."
			<< ((ip_value & 0x00ff0000) >> 16) << "."
			<< ((ip_value & 0x0000ff00) >> 8) << "."
			<< (ip_value & 0x000000ff);

		return str.str();
	}

	inline void SplitString(std::vector<std::string>& out, const std::string& str, const std::string& delim)
	{
		std::string s0(str);
		s0 += delim;//扩展字符串以方便操作
		size_t size = s0.size();

		std::string::size_type pos;
		for (size_t i = 0; i < size; ++i)
		{
			pos = s0.find(delim, i);
			if (pos < size)
			{
				std::string s = s0.substr(i, pos - i);
				out.push_back(s);
				i = pos + delim.size() - 1;
			}
		}
	}
}
