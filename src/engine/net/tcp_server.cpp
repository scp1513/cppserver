#include "tcp_server.h"
#include "internal-header.h"
#include "scheduler.h"
#include "internal-scheduler.h"
#include <map>
#include <mutex>
#include <vector>

using namespace net;
using namespace asio;
using namespace asio::ip;
using namespace std::placeholders;

typedef TCPServer::OnConnectedHandler OnConnectedHandler;
typedef TCPServer::OnCloseHandler     OnCloseHandler;
typedef TCPServer::OnRecvHandler      OnRecvHandler;

/////////////////////////////////////////////////////////////////////////////
struct CoreShare
{
	Serial& serial;

	OnConnectedHandler onConnectedHandler;
	OnCloseHandler     onCloseHandler;
	OnRecvHandler      onRecvHandler;

	bool tcp_nodelay;
	uint send_buffer_size;
	uint recv_buffer_size;

	std::function<bool(uint)> Close;

	CoreShare(Serial& serial)
		: serial(serial)
		, tcp_nodelay(false)
		, send_buffer_size(32 * 1024)
		, recv_buffer_size(16 * 1024)
	{}
};

/////////////////////////////////////////////////////////////////////////////
class TCPServerSession : public std::enable_shared_from_this<TCPServerSession>
{
public:
	typedef std::shared_ptr<TCPServerSession> Ptr;

	TCPServerSession(CoreShare& core, uint connID, io_service& service);
	~TCPServerSession() {}
	tcp::socket& GetSocket() { return mSocket; }
	uint GetConnID() { return mConnID; }
	void Start();
	size_t Send(const void* data, size_t len);
	bool Close();

private:
	void recv_len();
	void handle_recv_len(std::error_code ec, std::size_t bytes);
	void handle_recv_body(std::error_code ec, std::size_t bytes);
	void packet_handler(char* buf, uint length);

private:
	CoreShare&        mCore;
	uint              mConnID;
	tcp::socket       mSocket;
	std::vector<byte> mBuffer;
};

/////////////////////////////////////////////////////////////////////////////
struct TCPServer::Core
{
	CoreShare share;

	io_service& service;

	std::string ip;
	int         port;

	uint idCounter;
	std::mutex mutex;
	std::map<uint, TCPServerSession::Ptr> sessions;

	std::shared_ptr<tcp::acceptor> acceptor_;
	tcp::endpoint endpoint_;

	Core(Serial& serial, io_service& service)
		: share(serial)
		, service(service)
		, port(0)
		, idCounter(0)
	{
		share.Close = std::bind(&Core::Close, this, _1);
	}

	bool StartAccept();
	void HandleAccept(TCPServerSession::Ptr session, std::error_code ec);
	bool Close(uint connID);
};

/////////////////////////////////////////////////////////////////////////////
bool TCPServer::Core::StartAccept()
{
	try
	{
		TCPServerSession::Ptr session(new TCPServerSession(share, ++idCounter, service));
		auto& socket = session->GetSocket();
		auto handler = std::bind(&Core::HandleAccept, this, session, _1);
		acceptor_->async_accept(socket, handler);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void TCPServer::Core::HandleAccept(TCPServerSession::Ptr session, std::error_code ec)
{
	if (!ec)
	{
		bool insertSuccess = false;
		{
			std::lock_guard<std::mutex> guard(mutex);
			insertSuccess = sessions.insert(std::make_pair(session->GetConnID(), session)).second;
		}
		if (insertSuccess)
		{
			share.serial.Post(std::bind(share.onConnectedHandler, session->GetConnID()));
			session->Start();
		}
	}

	StartAccept();
}

bool TCPServer::Core::Close(uint connID)
{
	try
	{
		std::lock_guard<std::mutex> guard(mutex);
		auto iter = sessions.find(connID);
		if (iter == sessions.end()) return false;
		iter->second->Close();
		sessions.erase(iter);
		share.serial.Post(std::bind(share.onCloseHandler, connID));
		return true;
	}
	catch (...)
	{
		return false;
	}
}

/////////////////////////////////////////////////////////////////////////////

TCPServerSession::TCPServerSession(CoreShare& core, uint connID, io_service & service)
	: mCore(core)
	, mConnID(connID)
	, mSocket(service)
{
}

inline void TCPServerSession::Start()
{
	mBuffer.resize(sizeof(uint16));

	mSocket.set_option(tcp::no_delay(mCore.tcp_nodelay));
	mSocket.set_option(tcp::socket::keep_alive(false));
	mSocket.set_option(tcp::socket::send_buffer_size(mCore.send_buffer_size));
	mSocket.set_option(tcp::socket::receive_buffer_size(mCore.recv_buffer_size));

	recv_len();
}

inline size_t TCPServerSession::Send(const void * data, size_t len)
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
		return 0;
	}
}

inline bool TCPServerSession::Close()
{
	try
	{
		if (!mSocket.is_open()) return false;
		mSocket.shutdown(socket_base::shutdown_both);
		mSocket.close();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

inline void TCPServerSession::recv_len()
{
	try
	{
		auto handler = std::bind(&TCPServerSession::handle_recv_len, shared_from_this(), _1, _2);
		asio::async_read(mSocket, asio::buffer(&mBuffer[0], sizeof(uint16)), handler);
		//mSocket.async_receive(asio::buffer(&mBuffer[0], sizeof(uint16)), 0, handler);
	}
	catch (...)
	{
	}
}

inline void TCPServerSession::handle_recv_len(std::error_code ec, std::size_t bytes)
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
		mCore.Close(GetConnID());
		return;
	}

	try
	{
		// 包总长 + 包类型（单包/多包）
		uint16 length = (uint16)(uint8)mBuffer[0] | ((uint16)(uint8)mBuffer[1] << 8);
		if (mBuffer.size() < size_t(length + 2))
			mBuffer.resize(length + 2);

		auto handler = std::bind(&TCPServerSession::handle_recv_body, shared_from_this(), _1, _2);
		asio::async_read(mSocket, asio::buffer(&mBuffer[0], length + 2), handler);
	}
	catch (...)
	{
	}
}

inline void TCPServerSession::handle_recv_body(std::error_code ec, std::size_t bytes)
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
		mCore.Close(GetConnID());
		return;
	}

	try
	{
		char* packet = new char[bytes];
		memcpy(packet, &mBuffer[0], bytes);
		mCore.serial.Post(std::bind(&TCPServerSession::packet_handler, shared_from_this(), packet, bytes));
	}
	catch (...)
	{
	}
	recv_len();
}

inline void TCPServerSession::packet_handler(char * buf, uint length)
{
	// 包总长 + 包类型（单包/多包）
	uint16 label = (uint16)(uint8)buf[0] | ((uint16)(uint8)buf[1] << 8);

	if (label == 24)
	{
		mCore.onRecvHandler(GetConnID(), buf + 2, length - 2);
	}
	else
	{
		// multi packet
		// |--label(2)--|--num(1)--|--single len--|--single packet--|...
		uint8 count = *(buf + 2);

		uint index = 3;
		for (uint i = 0; i < count; ++i)
		{
			if (index + 1 >= length)
				return;

			uint16 singleLen = (uint16)(uint8)buf[index] | ((uint16)(uint8)buf[index + 1] << 8);

			if (index + 2 + singleLen > length)
				return;

			mCore.onRecvHandler(GetConnID(), &buf[index + 2], singleLen);
			index += 2 + singleLen;
		}
	}

	delete[] buf;
}

/////////////////////////////////////////////////////////////////////////////

TCPServer::TCPServer(const Params& params)
{
	auto& serial = Scheduler::GetInstance().GetSerial();
	auto& service = Scheduler::GetInstance().mCore->service;
	mCore.reset(new Core(serial, service));
	mCore->ip = params.ip;
	mCore->port = params.port;
	mCore->share.onConnectedHandler = params.onconnected_handler;
	mCore->share.onRecvHandler = params.onrecv_handler;
	mCore->share.onCloseHandler = params.onclose_handler;

	mCore->endpoint_ = tcp::endpoint(address::from_string(params.ip), params.port);
}

TCPServer::~TCPServer()
{
}

uint TCPServer::GetConnCount() const
{
	std::lock_guard<std::mutex> guard(mCore->mutex);
	return mCore->sessions.size();
}

std::string TCPServer::GetIPAddr(uint connID)
{
	try
	{
		std::lock_guard<std::mutex> guard(mCore->mutex);
		auto iter = mCore->sessions.find(connID);
		if (iter == mCore->sessions.end()) return std::string();
		return iter->second->GetSocket().remote_endpoint().address().to_string();
	}
	catch (...)
	{
		return std::string();
	}
}

bool TCPServer::Start()
{
	try
	{
		mCore->acceptor_.reset(new tcp::acceptor(mCore->service, mCore->endpoint_));

		//acceptor_->set_option(tcp::acceptor::reuse_address(false));
		//acceptor_->bind(endpoint_);
		//acceptor_->listen();

		return mCore->StartAccept();
	}
	catch (...)
	{
		return false;
	}
	return false;
}

void TCPServer::Stop()
{
	try
	{
		std::lock_guard<std::mutex> guard(mCore->mutex);
		mCore->acceptor_->cancel();
		for (auto & pair : mCore->sessions)
		{
			pair.second->Close();
		}
	}
	catch (...)
	{
	}
}

int TCPServer::Send(uint connID, const void *data, size_t len)
{
	try
	{
		std::lock_guard<std::mutex> guard(mCore->mutex);
		auto iter = mCore->sessions.find(connID);
		if (iter == mCore->sessions.end()) return 0;
		return iter->second->Send(data, len);
	}
	catch (...)
	{
		return 0;
	}
}

bool TCPServer::Close(uint connID)
{
	return mCore->Close(connID);
}

void TCPServer::SetTCPNoDelay(bool val)
{
	mCore->share.tcp_nodelay = val;
}

void TCPServer::SetSendBufSize(uint val)
{
	mCore->share.send_buffer_size = val;
}

void TCPServer::SetRecvBufSize(uint val)
{
	mCore->share.recv_buffer_size = val;
}

bool TCPServer::GetTCPNoDelay()
{
	return mCore->share.tcp_nodelay;
}

uint TCPServer::GetSendBufSize()
{
	return mCore->share.send_buffer_size;
}

uint TCPServer::GetRecvBufSize()
{
	return mCore->share.recv_buffer_size;
}
