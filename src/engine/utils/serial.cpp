#include "serial.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <map>

// _WIN32_WINNT version constants
#define _WIN32_WINNT_NT4          0x0400 // Windows NT 4.0
#define _WIN32_WINNT_WIN2K        0x0500 // Windows 2000
#define _WIN32_WINNT_WINXP        0x0501 // Windows XP
#define _WIN32_WINNT_WS03         0x0502 // Windows Server 2003
#define _WIN32_WINNT_WIN6         0x0600 // Windows Vista
#define _WIN32_WINNT_VISTA        0x0600 // Windows Vista
#define _WIN32_WINNT_WS08         0x0600 // Windows Server 2008
#define _WIN32_WINNT_LONGHORN     0x0600 // Windows Vista
#define _WIN32_WINNT_WIN7         0x0601 // Windows 7
#define _WIN32_WINNT_WIN8         0x0602 // Windows 8
#define _WIN32_WINNT_WINBLUE      0x0603 // Windows 8.1
#define _WIN32_WINNT_WINTHRESHOLD 0x0A00 // Windows 10
#define _WIN32_WINNT_WIN10        0x0A00 // Windows 10

#define _WIN32_WINNT _WIN32_WINNT_WIN10

#define ASIO_STANDALONE
#define ASIO_HAS_STD_CHRONO

#define BOOST_DATE_TIME_NO_LIB
#define BOOST_REGEX_NO_LIB

#include <asio.hpp>
#include <asio/io_service.hpp>
#include <asio/system_timer.hpp>

using namespace asio;
using namespace std::chrono;
using namespace std::placeholders;

struct TimerInfo
{
	uint timerID;
	system_clock::duration duration;
	system_timer timer;

	TimerInfo(io_service& service) : timer(service) {}
};

typedef std::shared_ptr<TimerInfo> TimerInfoPtr;

struct Serial::Core
{
	io_service service;
	std::shared_ptr<io_service::work> serviceWork;

	std::thread thread;
	std::mutex mutex;

	bool working;
	uint timerIDCounter;

	std::map<uint, TimerInfoPtr> timerInfos;

	Core() : working(false), timerIDCounter(0) {}
};

static void _TimerHandler(TimerInfoPtr timerInfo, const Serial::TimerHandler& handler, const asio::error_code& err)
{
	if (err) return; // already cancel
	handler(timerInfo->timerID);
	auto t = timerInfo->timer.expires_at() + timerInfo->duration;
	timerInfo->timer.expires_at(t);
	timerInfo->timer.async_wait(std::bind(_TimerHandler, timerInfo, handler, _1));
}

static void _ExpireHandler(TimerInfoPtr timerInfo, const Serial::TimerHandler& handler, Serial* serial, const asio::error_code& err)
{
	if (err) return; // already cancel
	handler(timerInfo->timerID);
	serial->RemoveTimer(timerInfo->timerID);
}

Serial::Serial()
	: mCore(new Core())
{
}

Serial::~Serial()
{
}

void Serial::Start()
{
	mCore->working = true;
	mCore->serviceWork.reset(new io_service::work(mCore->service));
	mCore->thread.swap(std::thread([&]() { mCore->service.run(); }));
}

void Serial::Stop()
{
	{
		std::lock_guard<std::mutex> guard(mCore->mutex);
		mCore->working = false;
		for (auto& kv : mCore->timerInfos)
		{
			kv.second->timer.cancel();
		}
	}
	mCore->serviceWork.reset();
	mCore->service.stop();
	if (mCore->thread.joinable())
		mCore->thread.join();
}

void Serial::Post(const PostHandler& handler)
{
	if (handler == nullptr) return;
	mCore->service.post(handler);
}

uint Serial::AddTimer(const std::chrono::system_clock::duration& duration, const TimerHandler& handler)
{
	if (handler == nullptr) return 0;

	auto eventTime = system_clock::now() + duration;

	TimerInfoPtr timerInfo(new TimerInfo(mCore->service));
	timerInfo->duration = duration;

	{
		std::lock_guard<std::mutex> guard(mCore->mutex);
		timerInfo->timerID = ++mCore->timerIDCounter;
		mCore->timerInfos[timerInfo->timerID] = timerInfo;
	}

	timerInfo->timer.expires_at(eventTime);
	timerInfo->timer.async_wait(std::bind(_TimerHandler, timerInfo, handler, _1));
	return timerInfo->timerID;
}

uint Serial::Expire(const std::chrono::system_clock::duration& duration, const TimerHandler& handler)
{
	if (handler == nullptr) return 0;

	TimerInfoPtr timerInfo(new TimerInfo(mCore->service));
	timerInfo->duration = duration;
	timerInfo->timer.expires_from_now(duration);

	{
		std::lock_guard<std::mutex> guard(mCore->mutex);
		timerInfo->timerID = ++mCore->timerIDCounter;
		mCore->timerInfos[timerInfo->timerID] = timerInfo;
	}

	timerInfo->timer.async_wait(std::bind(_ExpireHandler, timerInfo, handler, this, _1));
	return timerInfo->timerID;
}

uint Serial::ExpireAt(const std::chrono::system_clock::time_point& time, const TimerHandler& handler)
{
	auto now = system_clock::now();
	if (time < now) return 0;
	return Expire(time - now, handler);
}

void Serial::RemoveTimer(uint index)
{
	TimerInfoPtr timerInfo;
	{
		std::lock_guard<std::mutex> guard(mCore->mutex);
		auto iter = mCore->timerInfos.find(index);
		if (iter == mCore->timerInfos.end()) return;
		timerInfo = iter->second;
		mCore->timerInfos.erase(iter);
	}
	timerInfo->timer.cancel();
}
