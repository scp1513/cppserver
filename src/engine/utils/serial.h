#ifndef __UTILS_SERIAL_HEADER_FILE__
#define __UTILS_SERIAL_HEADER_FILE__

#include <utils/typedef.h>
#include <functional>
#include <memory>
#include <chrono>

class Serial
{
public:
	typedef std::function<void(uint)> TimerHandler;
	typedef std::function<void(void)> PostHandler;

	Serial();

	~Serial();

	void Start();

	void Stop();

	void Post(const PostHandler& handler);

	uint AddTimer(const std::chrono::system_clock::duration& duration, const TimerHandler& handler);

	uint AddTimer(uint millisec, const TimerHandler& handler) { return AddTimer(std::chrono::milliseconds(millisec), handler); }

	uint Expire(const std::chrono::system_clock::duration& duration, const TimerHandler& handler);

	uint Expire(uint millisec, const TimerHandler& handler) { return Expire(std::chrono::milliseconds(millisec), handler); }

	uint ExpireAt(const std::chrono::system_clock::time_point& time, const TimerHandler& handler);

	void RemoveTimer(uint timerID);

private:
	struct Core;
	std::shared_ptr<Core> mCore;
};

#endif