#ifndef __NET_TCPSERVER_HEADER__
#define __NET_TCPSERVER_HEADER__

#include <utils/typedef.h>
#include <functional>
#include <memory>
#include <string>

namespace net
{
	class TCPServer
	{
	public:
		typedef std::function<void(uint)>                      OnConnectedHandler;
		typedef std::function<void(uint)>                      OnCloseHandler;
		typedef std::function<void(uint, const void*, size_t)> OnRecvHandler;

		struct Params
		{
			std::string        ip;
			int                port;
			OnConnectedHandler onconnected_handler;
			OnCloseHandler     onclose_handler;
			OnRecvHandler      onrecv_handler;
		};

	public:
		TCPServer(const Params& params);
		~TCPServer();

		uint GetConnCount() const;
		std::string GetIPAddr(uint connID);

		bool Start();
		void Stop();

		int Send(uint connID, const void* data, size_t len);
		bool Close(uint connID);

		void SetTCPNoDelay(bool val);
		void SetSendBufSize(uint val);
		void SetRecvBufSize(uint val);
		bool GetTCPNoDelay();
		uint GetSendBufSize();
		uint GetRecvBufSize();

	private:
		struct Core;
		std::shared_ptr<Core> mCore;
	};
}

#endif