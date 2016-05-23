#ifndef __UTILS_SINGLETON_HEADER__
#define __UTILS_SINGLETON_HEADER__

#include <mutex>
#include <memory>

namespace utils
{
	template<typename T>
	class Singleton
	{
	public:
		static T& GetInstance()
		{
			if (t_ == nullptr)
			{
				std::call_once(flag_, []()
				{
					if (t_ == nullptr)
						t_.reset(new T());
				});
			}
			return *t_.get();
		}

	protected:
		Singleton() {}
		~Singleton() {}

	private:
		Singleton(const Singleton&);
		const Singleton& operator=(const Singleton&);

	private:
		static std::shared_ptr<T> t_;
		static std::once_flag flag_;
	};

	template<class T> std::shared_ptr<T> Singleton<T>::t_;
	template<class T> std::once_flag Singleton<T>::flag_;
}


#endif