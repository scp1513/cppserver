#ifndef __UTILS_SERIAL_HEADER_FILE__
#define __UTILS_SERIAL_HEADER_FILE__

#include <utils/typedef.h>
#include <functional>
#include <memory>
#include <chrono>
#include <ctime>

class Serial
{
public:
	struct Core;
	typedef std::function<void(uint)> TimerHandler;
	typedef std::function<void(void)> PostHandler;

	Serial();

	~Serial();

	void Start();

	void Stop();

	void Post(const PostHandler& handler);

	uint AddTimer(uint millisec, const TimerHandler& handler);

	uint AddTimer(const std::chrono::system_clock::duration& duration, const TimerHandler& handler);

	uint Expire(const std::chrono::system_clock::duration& duration, const TimerHandler& handler);

	uint Expire(uint millisec, const TimerHandler& handler);

	uint ExpireAt(const std::chrono::system_clock::time_point& time, const TimerHandler& handler);

	uint ExpireAt(const std::tm& tm, const TimerHandler& handler);

	uint ExpireAt(const std::time_t& t, const TimerHandler& handler);

	void RemoveTimer(uint timerID);

private:
	std::shared_ptr<Core> mCore;
};

#endif