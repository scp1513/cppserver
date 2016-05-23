#ifndef __UTILS_VARIABLE_HEADER__
#define __UTILS_VARIABLE_HEADER__

#include <utils/typedef.h>
#include <limits>

namespace utils
{

#undef min
#undef max

	enum
	{
		VAR_FLAG_OK,
		VAR_FLAG_UNDERFLOW,
		VAR_FLAG_OVERFLOW,
	};

	template<typename T>
	class Variable
	{
	public:
		typedef unsigned char FlagType;

		Variable(T value = 0, bool stick_to_limits = true)
			: flag_(VAR_FLAG_OK)
			, value_(value)
			, stick_to_limits_(stick_to_limits)
		{
			max_ = std::numeric_limits<T>::max();
			min_ = std::numeric_limits<T>::min();
		}

		FlagType GetFlag() const { return flag_; }
		void ResetFlag() { flag_ = VAR_FLAG_OK; }
		operator T() const { return value_; }

		template<typename Tvalue>
		void operator =(Tvalue oprand)
		{
			ResetFlag();

			if (oprand > max_)
			{
				flag_ = VAR_FLAG_OVERFLOW;
				if (stick_to_limits_)
				{
					value_ = max_;
				}
			}
			else if (oprand < min_)
			{
				flag_ = VAR_FLAG_UNDERFLOW;
				if (stick_to_limits_)
				{
					value_ = min_;
				}
			}
			else
			{
				value_ = oprand;
			}
		}

		template<typename Tvalue>
		void operator +=(Tvalue oprand)
		{
			ResetFlag();

			if (oprand >= 0)
			{
				if (static_cast<uint64>(oprand) > static_cast<uint64>(max_ - value_))
				{
					flag_ = VAR_FLAG_OVERFLOW;
					if (stick_to_limits_)
					{
						value_ = max_;
					}
				}
			}
			else
			{
				if (static_cast<uint64>(0 - oprand) > static_cast<uint64>(value_ - min_))
				{
					flag_ = VAR_FLAG_UNDERFLOW;
					if (stick_to_limits_)
					{
						value_ = min_;
					}
				}
			}

			if (flag_ == VAR_FLAG_OK)
			{
				value_ += static_cast<T>(oprand);
			}
		}

		template<typename Tvalue>
		void operator -=(Tvalue oprand)
		{
			ResetFlag();
			if (oprand >= 0)
			{
				if (static_cast<uint64>(oprand) > static_cast<uint64>(value_ - min_))
				{
					flag_ = VAR_FLAG_UNDERFLOW;
					if (stick_to_limits_)
					{
						value_ = min_;
					}
				}
			}
			else
			{
				if (static_cast<uint64>(0 - oprand) > static_cast<uint64>(max_ - value_))
				{
					flag_ = VAR_FLAG_OVERFLOW;
					if (stick_to_limits_)
					{
						value_ = max_;
					}
				}
			}

			if (flag_ == VAR_FLAG_OK)
			{
				value_ -= static_cast<T>(oprand);
			}
		}

	private:
		FlagType flag_;

		T value_;
		T min_;
		T max_;
		bool stick_to_limits_;
	};

	template <typename T>
	class StrictVal
	{
	public:
		typedef unsigned char FlagType;
		StrictVal(T value = 0)
			: flag_(VAR_FLAG_OK)
			, value_(value)
		{
			max_ = std::numeric_limits<T>::max();
			min_ = std::numeric_limits<T>::min();
		}

		FlagType GetFlag() const { return flag_; }
		void ResetFlag() { flag_ = VAR_FLAG_OK; }
		operator T() const { return value_; }

		template<typename Tvalue>
		void operator =(Tvalue oprand)
		{
			ResetFlag();

			if (oprand > max_)
			{
				flag_ = VAR_FLAG_OVERFLOW;
				value_ = max_;
			}
			else if (oprand < min_)
			{
				flag_ = VAR_FLAG_UNDERFLOW;
				value_ = min_;
			}
			else
			{
				value_ = oprand;
			}
		}

		template<typename Tvalue>
		void operator +=(Tvalue oprand)
		{
			ResetFlag();

			if (oprand >= 0)
			{
				if (static_cast<uint64>(oprand) > static_cast<uint64>(max_ - value_))
				{
					flag_ = VAR_FLAG_OVERFLOW;
					value_ = max_;
				}
			}
			else
			{
				if (static_cast<uint64>(0 - oprand) > static_cast<uint64>(value_ - min_))
				{
					flag_ = VAR_FLAG_UNDERFLOW;
					value_ = min_;
				}
			}

			if (flag_ == VAR_FLAG_OK)
			{
				value_ += static_cast<T>(oprand);
			}
		}

		template<typename Tvalue>
		void operator -=(Tvalue oprand)
		{
			ResetFlag();
			if (oprand >= 0)
			{
				if (static_cast<uint64>(oprand) > static_cast<uint64>(value_ - min_))
				{
					flag_ = VAR_FLAG_UNDERFLOW;
					value_ = min_;
				}
			}
			else
			{
				if (static_cast<uint64>(0 - oprand) > static_cast<uint64>(max_ - value_))
				{
					flag_ = VAR_FLAG_OVERFLOW;
					value_ = max_;
				}
			}

			if (flag_ == VAR_FLAG_OK)
			{
				value_ -= static_cast<T>(oprand);
			}
		}

	private:
		FlagType flag_;

		T value_;
		T min_;
		T max_;
	};
}

#endif