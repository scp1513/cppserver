#ifndef __NET_INTERNAL_SCHEDULER_HEADER__
#define __NET_INTERNAL_SCHEDULER_HEADER__

#include "internal-header.h"
#include <utils/serial.h>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace net
{
	class Scheduler;
	struct Scheduler::Core
	{
		bool working;
		Serial serial;

		asio::io_service service;
		std::shared_ptr<asio::io_service::work> work;

		std::mutex mutex;
		std::vector<std::thread> threads;

		Core() : working(false), threads(1) {}
	};
}

#endif