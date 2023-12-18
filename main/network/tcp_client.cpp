#include "tcp_client.h"

#include <sys/socket.h>
#include <netdb.h>
#include <cstdio>

namespace net
{
	TCPClient::TCPClient(port_t port)
		: _id(-1), _port(port)
	{
	}

	TCPClient::~TCPClient()
	{
		Close();
	}

	TCPError TCPClient::Open()
	{
		if constexpr(USE_IPV4)
		{
			_id = socket(AF_INET, SOCK_STREAM, 0);
		} else
		{
			_id = socket(AF_INET6, SOCK_STREAM, 0);
		}

		if(_id < 0)
		{
			return TCPError::UNABLE_TO_OPEN_SOCKET;
		}
		return TCPError::NO_ERROR;
	}

	TCPError TCPClient::Connect(const char* ip)
	{
		sockaddr_in addr;
		if constexpr(USE_IPV4)
		{
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(ip);
		}
		else
		{
			addr.sin_family = AF_INET6;
		}
		addr.sin_port = htons(_port); // Port to listen on

		if(connect(_id, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
		{
			return TCPError::CONNECTING_FAILED;
		}
		return TCPError::NO_ERROR;
	}

	TCPError IRAM_ATTR TCPClient::Send(void const* data, size_t size_in_bytes) 
	{
		size_t send_bytes = 0;
		do
		{
			const int length = send(_id, data + send_bytes, size_in_bytes, 0);
			if(length < 0)
			{
				// Lost connection.
				return TCPError::SENDING_FAILED;
			}
			send_bytes += length;
			size_in_bytes -= length;
		} while(size_in_bytes);
		return TCPError::NO_ERROR;
	}

	int TCPClient::Receive(void* data, size_t size_in_bytes) const
	{
		return recv(_id, data, size_in_bytes, 0);
	}

	void TCPClient::SetTimeout(long const& s, long const& us)
	{
		timeval timeout{};
		timeout.tv_sec  = s;
		timeout.tv_usec = us;
		setsockopt(_id, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeval));
	}

	bool TCPClient::IsConnected()
	{
		if(_id == -1)
			return false;

		int error;
		int errorSize= sizeof(error);
		int sockOptError = getsockopt(_id, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&errorSize);
		return !(sockOptError || error);
	}

	void TCPClient::Close()
	{
		if(_id >= 0)
		{
			shutdown(_id, 0);
			close(_id);
			_id = -1;
		}
	}
	
}
