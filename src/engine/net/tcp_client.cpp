#include "tcp_client.h"
#include "internal-header.h"
#include "scheduler.h"
#include "internal-scheduler.h"
#include <map>
#include <mutex>
#include <memory>
#include <vector>
#include <sstream>

using namespace net;
using namespace asio;
using namespace asio::ip;
using namespace std::placeholders;

/////////////////////////////////////////////////////////////////////////////
class TCPClientSession : public std::enable_shared_from_this<TCPClientSession>
{
public:
	typedef std::shared_ptr<TCPClientSession> Ptr;

	typedef TCPClient::OnConnectionHandler OnConnectionHandler;
	typedef TCPClient::OnRecvHandler       OnRecvHandler;
	typedef TCPClient::OnCloseHandler      OnCloseHandler;

	TCPClientSession(
		TCPClient*          mgr,
		uint                connID,
		Serial&				serial,
		io_service&         service,
		OnConnectionHandler onConnection,
		OnCloseHandler      onClose,
		OnRecvHandler       onRecv)
		: mMgr(mgr)
		, mSerial(serial)
		, mService(service)
		, mConnID(connID)
		, mSocket(service)
		, mResolver(service)
		, mOnConnectionHandler(onConnection)
		, mOnRecvHandler(onRecv)
		, mOnCloseHandler(onClose) {}

	~TCPClientSession() {}

	tcp::socket& getSocket() { return mSocket; }
	uint getConnID() { return mConnID; }

	void resolve(const std::string& ip, const std::string& port)
	{
		try
		{
			tcp::resolver::query query(ip, port);
			auto handler = std::bind(&TCPClientSession::handle_resolve, shared_from_this(), _1, _2);
			mResolver.async_resolve(query, handler);
		}
		catch (...)
		{
		}
	}

	size_t send(const void* data, size_t len)
	{
		try
		{
			byte buf[4];
			buf[0] = byte(len);
			buf[1] = byte(len >> 8);
			buf[2] = 24;
			buf[3] = 0;

			asio::write(mSocket, asio::buffer(buf, 4));
			return asio::write(mSocket, asio::buffer(data, len));
		}
		catch (...)
		{
		}
		return 0;
	}

	void close()
	{
		try
		{
			if (!mSocket.is_open()) return;
			mSocket.shutdown(socket_base::shutdown_both);
		}
		catch (...)
		{
		}
	}

	void postClosedHandler()
	{
		mSerial.Post(std::bind(mOnCloseHandler, getConnID()));
	}

private:
	void handle_resolve(const std::error_code& ec, tcp::resolver::iterator endpoint_iterator)
	{
		try
		{
			if (ec)
			{
				mSerial.Post(std::bind(mOnConnectionHandler, getConnID(), TCPClient::Result::AddrResolveFailed, ec.message()));
				return;
			}

			mSerial.Post(std::bind(mOnConnectionHandler, getConnID(), TCPClient::Result::AddrResolveSuccessed, ec.message()));

			connect(endpoint_iterator);
		}
		catch (...)
		{
		}
	}

	void connect(const tcp::resolver::iterator& endpoint_iterator)
	{
		try
		{
			mSocket.async_connect(*endpoint_iterator, std::bind(&TCPClientSession::handle_connect, shared_from_this(), _1));
		}
		catch (...)
		{
		}
	}

	void handle_connect(std::error_code ec)
	{
		try
		{
			if (ec)
			{
				mSerial.Post(std::bind(mOnConnectionHandler, getConnID(), TCPClient::Result::ConnectionFailed, ec.message()));
				return;
			}

			mBuffer.resize(sizeof(unsigned short));

			mSocket.set_option(tcp::no_delay(true));
			mSocket.set_option(tcp::socket::keep_alive(false));
			mSocket.set_option(tcp::socket::send_buffer_size(32 * 1024));
			mSocket.set_option(tcp::socket::receive_buffer_size(16 * 1024));

			mSerial.Post(std::bind(mOnConnectionHandler, getConnID(), TCPClient::Result::ConnectionSuccessed, ec.message()));

			recv_len();
		}
		catch (...)
		{
		}
	}

	void recv_len()
	{
		try
		{
			auto handler = std::bind(&TCPClientSession::handle_recv_len, shared_from_this(), _1, _2);
			asio::async_read(mSocket, asio::buffer(&mBuffer[0], sizeof(uint16)), handler);
		}
		catch (...)
		{
		}
	}

	void handle_recv_len(std::error_code ec, std::size_t bytes)
	{
		if (ec)
		{
			switch (ec.value())
			{
			case asio::error::eof:
				// 远端正常关闭socket
				break;
			case asio::error::connection_reset:
				// 远端暴力关闭socket
				break;
			case asio::error::operation_aborted:
				// 正在async_receive()异步任务等待时，本端关闭套接字
				break;
			case asio::error::bad_descriptor:
				// 在一个已经关闭了的套接字上执行async_receive()
				break;
			default:
				break;
			}

			postClosedHandler();
			return;
		}

		try
		{
			// 包总长 + 包类型（单包/多包）
			uint16 length = (uint16)(uint8)mBuffer[0] | ((uint16)(uint8)mBuffer[1] << 8);
			if (mBuffer.size() < size_t(length + 2))
				mBuffer.resize(length + 2);

			auto handler = std::bind(&TCPClientSession::handle_recv_body, shared_from_this(), _1, _2);
			asio::async_read(mSocket, asio::buffer(&mBuffer[0], length + 2), handler);
		}
		catch (...)
		{
		}
	}

	void handle_recv_body(std::error_code ec, std::size_t bytes)
	{
		if (ec)
		{
			switch (ec.value())
			{
			case asio::error::eof:
				// 远端正常关闭socket
				break;
			case asio::error::connection_reset:
				// 远端暴力关闭socket
				break;
			case asio::error::operation_aborted:
				// 正在async_receive()异步任务等待时，本端关闭套接字
				break;
			case asio::error::bad_descriptor:
				// 在一个已经关闭了的套接字上执行async_receive()
				break;
			default:
				break;
			}
			postClosedHandler();

			return;
		}

		try
		{
			char* packet = new char[bytes];
			memcpy(packet, &mBuffer[0], bytes);
			mSerial.Post(std::bind(&TCPClientSession::packet_handler, shared_from_this(), packet, bytes));
		}
		catch (...)
		{
		}
		recv_len();
	}

	void packet_handler(char* buf, uint length)
	{
		// 包总长 + 包类型（单包/多包）
		uint16 label = (uint16)(uint8)buf[0] | ((uint16)(uint8)buf[1] << 8);

		if (label == 24)
		{
			mOnRecvHandler(getConnID(), buf + 2, length - 2);
		}
		else
		{
			// multi packet
			// |--label(2)--|--num(1)--|--single len--|--single packet...--|
			uint8 count = *(buf + 2);

			uint index = 3;
			for (uint i = 0; i < count; ++i)
			{
				if (index + 1 >= length)
					return;

				uint16 single_len = (uint16)(uint8)buf[index] | ((uint16)(uint8)buf[index + 1] << 8);

				if (index + 2 + single_len > length)
					return;

				mOnRecvHandler(getConnID(), &buf[index + 2], single_len);
				index += 2 + single_len;
			}
		}

		delete[] buf;
	}

private:
	TCPClient* mMgr;
	Serial& mSerial;
	io_service& mService;

	uint mConnID;

	tcp::socket   mSocket;
	tcp::resolver mResolver;

	OnConnectionHandler mOnConnectionHandler;
	OnRecvHandler       mOnRecvHandler;
	OnCloseHandler      mOnCloseHandler;

	std::vector<byte> mBuffer;
};

/////////////////////////////////////////////////////////////////////////////
struct TCPClient::Core
{
	Serial& serial;
	io_service& service;
	uint idCounter;

	bool tcp_nodelay;
	uint send_buffer_size;
	uint recv_buffer_size;

	std::mutex sessions_mutex;
	std::map<uint, TCPClientSession::Ptr> sessions;

	Core(Serial& serial, io_service& service)
		: serial(serial)
		, service(service)
		, idCounter(0)
		, tcp_nodelay(true)
		, send_buffer_size(32 * 1024)
		, recv_buffer_size(16 * 1924) {}
};

/////////////////////////////////////////////////////////////////////////////
TCPClient::TCPClient()
{
	auto& serial = Scheduler::GetInstance().GetSerial();
	auto& service = Scheduler::GetInstance().mCore->service;
	mCore.reset(new TCPClient::Core(serial, service));
}

TCPClient::~TCPClient()
{
}

uint TCPClient::ConnectTo(const ConnectParams& params)
{
	try
	{
		auto & service = mCore->service;
		TCPClientSession::Ptr session(new TCPClientSession(
			this,
			++mCore->idCounter,
			mCore->serial,
			service,
			params.onConnectionHandler,
			params.onCloseHandler,
			params.onRecvHandler));

		if (!mCore->sessions.insert(std::make_pair(session->getConnID(), session)).second)
			return 0;

		std::stringstream ss;
		ss << params.port;
		session->resolve(params.ip, ss.str());

		return session->getConnID();
	}
	catch (...)
	{
		return 0;
	}
}

void TCPClient::Disconnect(uint connID)
{
	try
	{
		std::unique_lock<std::mutex> lock(mCore->sessions_mutex);
		auto iter = mCore->sessions.find(connID);
		if (iter == mCore->sessions.end()) return;
		//iter->second->postClosedHandler();
		iter->second->close();
	}
	catch (...)
	{
	}
}

size_t TCPClient::Send(uint connID, const void* data, size_t len)
{
	try
	{
		std::unique_lock<std::mutex> lock(mCore->sessions_mutex);
		auto iter = mCore->sessions.find(connID);
		if (iter == mCore->sessions.end()) return 0;
		return iter->second->send(data, len);
	}
	catch (...)
	{
		return 0;
	}
}

void TCPClient::SetTCPNoDelay(bool val)
{
	mCore->tcp_nodelay = val;
}

void TCPClient::SetSendBufSize(uint val)
{
	mCore->send_buffer_size = val;
}

void TCPClient::SetRecvBufSize(uint val)
{
	mCore->recv_buffer_size = val;
}

bool TCPClient::GetTCPNoDelay()
{
	return mCore->tcp_nodelay;
}

uint TCPClient::GetSendBufSize()
{
	return mCore->send_buffer_size;
}

uint TCPClient::GetRecvBufSize()
{
	return mCore->recv_buffer_size;
}
