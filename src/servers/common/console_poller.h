#ifndef __COMMON_CONSOLE_POLLER_HEADER__
#define __COMMON_CONSOLE_POLLER_HEADER__

#include <utils/singleton.h>
#include <functional>
#include <thread>

class ConsolePoller : public utils::Singleton<ConsolePoller>
{
	friend class utils::Singleton<ConsolePoller>;
	ConsolePoller();

public:
	~ConsolePoller();

	bool Start(const std::function<void(const char*)>& handler);
	void Stop();

private:
	bool mIsRunning;
	std::thread mThread;
	std::function<void(const char*)> mHandler;
};

inline ConsolePoller& sConsolePoller()
{
	static auto& instance = ConsolePoller::GetInstance();
	return instance;
}

#endif