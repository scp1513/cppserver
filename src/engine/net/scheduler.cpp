#include "scheduler.h"
#include "internal-header.h"
#include "internal-scheduler.h"
//#include <utils/platform.h>
#include <mutex>
#include <map>
#include <thread>
#include <vector>

using namespace net;
using namespace asio;

//#ifdef PLATFORM_WIN32
//BOOL WINAPI ConsoleHandler(DWORD msgType)
//{
//	if (msgType == CTRL_C_EVENT)
//	{
//		return TRUE;
//	}
//	else if (msgType == CTRL_CLOSE_EVENT)
//	{
//		// Note: The system gives you very limited time to exit in this condition
//		return TRUE;
//	}
//
//	//Other messages:
//	//CTRL_BREAK_EVENT         Ctrl-Break pressed
//	//CTRL_LOGOFF_EVENT        User log off
//	//CTRL_SHUTDOWN_EVENT      System shutdown
//
//	return FALSE;
//}
//#endif

Scheduler::Scheduler()
	: mCore(new Core())
{
//#ifdef PLATFORM_WIN32
//	SetConsoleCtrlHandler(ConsoleHandler, TRUE);
//#endif
}

Scheduler::~Scheduler()
{
}

void Scheduler::Start()
{
	if (mCore->working) return;
	mCore->working = true;
	mCore->work.reset(new io_service::work(mCore->service));
	for (size_t i = 0; i < mCore->threads.size(); ++i)
	{
		mCore->threads[i].swap(std::thread([&]()
		{
			mCore->service.run();
		}));
	}
	mCore->serial.Start();
}

void Scheduler::Stop()
{
	if (!mCore->working) return;
	mCore->working = false;
	mCore->work.reset();
	mCore->service.stop();
	for (size_t i = 0; i < mCore->threads.size(); ++i)
	{
		if (mCore->threads[i].joinable())
			mCore->threads[i].join();
	}
	mCore->serial.Stop();
}

void net::Scheduler::SetWorkerNum(uint n)
{
	if (mCore->working) return;
	mCore->threads.resize(n);
}

uint net::Scheduler::GetWorkerNum()
{
	return mCore->threads.size();
}

Serial & net::Scheduler::GetSerial()
{
	return mCore->serial;
}
