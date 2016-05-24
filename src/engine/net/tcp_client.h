#ifndef __NET_TCP_CLIENT_HEADER__
#define __NET_TCP_CLIENT_HEADER__

#include <utils/typedef.h>
#include <utils/singleton.h>
#include <functional>
#include <string>
#include <memory>

namespace net
{
	class TCPClient : public utils::Singleton<TCPClient>
	{
		friend class utils::Singleton<TCPClient>;
		TCPClient();

	public:
		enum class Result
		{
			Null,                 // 没有错误
			CommonError,          // 
			AddrResolveFailed,    // 添加节点失败
			AddrResolveSuccessed, // 添加节点成功
			ConnectionFailed,     // 连接失败
			ConnectionSuccessed,  // 连接成功
		};

		typedef std::function<void(uint, Result, std::string)> OnConnectionHandler;
		typedef std::function<void(uint, const void*, size_t)> OnRecvHandler;
		typedef std::function<void(uint)>                      OnCloseHandler;

		struct ConnectParams
		{
			std::string         ip;
			int                 port;
			OnConnectionHandler onConnectionHandler;
			OnRecvHandler       onRecvHandler;
			OnCloseHandler      onCloseHandler;
		};

	public:
		~TCPClient();

		uint ConnectTo(const ConnectParams& params);
		void Disconnect(uint connID);
		size_t Send(uint connID, const void* data, size_t len);

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