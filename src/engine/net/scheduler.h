#ifndef __NET_SHEDULER_HEADER_FILE__
#define __NET_SHEDULER_HEADER_FILE__

#include <utils/typedef.h>
#include <utils/singleton.h>
#include <utils/serial.h>
#include <memory>

namespace net
{
	class Scheduler : public utils::Singleton<Scheduler>
	{
		friend class utils::Singleton<Scheduler>;
		friend class TCPClient;
		friend class TCPServer;

		struct Core;

		Scheduler();
	public:
		~Scheduler();

		void Start();

		void Stop();

		void SetWorkerNum(uint n);

		uint GetWorkerNum();

		Serial& GetSerial();

	private:
		std::shared_ptr<Core> mCore;
	};
}

#endif