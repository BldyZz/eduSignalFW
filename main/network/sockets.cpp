#include "sockets.h"

#include <cassert>
#include <cstring>
#include <netdb.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "../util/utils.h"
#include "../util/defines.h"
#include "wifi.hpp"

namespace net
{
	Socket::Socket()
		: _id(-1), _protocol(0), _port(0)
	{

	}

    void Socket::Open(Protocol protocol, port_t port, Domain domain)
    {
		_port = port;
		
		switch(protocol)
		{
        case Protocol::UDP:
			_protocol = SOCK_DGRAM;
			break;
        case Protocol::TCP:
			_protocol = SOCK_STREAM;
        	break;
		default:
			break;
        }
		
		switch (domain) {
        case Domain::IPv4:
			_address = sockaddr_in
			{
				.sin_family = AF_INET,
				.sin_port   = htons(port),
			};
			break;
        //	case Domain::IPv6:
		//		s.addr6 = sockaddr_in6
		//		{
		//			.sin_family6 = AF_INET6,
		//			.sin_port   = htons(port),
		//		};
        //		break;
        }

		_id = socket(_address.sin_family, _protocol, 0);
		if(_id < 0)
		{
			assert(0 && "Socket could not be created.");
		}
    }

	void Socket::Close()
	{
		//shutdown(_id, 0);
		close(_id);
	}

	void Socket::EstablishTCPConnection()
	{
		if(bind(_id, (struct sockaddr*)&_address, sizeof(_address)) != 0)
		{
			Close();
			return;
		}

		if(listen(_id, 5))
		{
			Close();
			return;
		}
	}

	SocketError Socket::Send(void const* data, util::size_t size_in_bytes)
	{
		const int err = IsTCP() ? send(_id, data, size_in_bytes, 0) : sendto(_id, data, size_in_bytes, 0, (const sockaddr*) & _address, sizeof(_address));

		if(err < 0) 
		{
			PRINTI("[Socket:]", "Sending package failed with msg: %s\n", strerror(errno));
			return SocketError::SENDING_FAILED;
		}
		return SocketError::NO_ERROR;
	}

	int Socket::Receive(OUT void* buffer, util::size_t size_in_bytes)
	{
		socklen_t socketAddressSize = sizeof(_lastReceiveAddress);

		auto length = recvfrom(_id, buffer, size_in_bytes, 0, reinterpret_cast<sockaddr*>(&_lastReceiveAddress), IN & socketAddressSize);
		if(length < 0)
		{
			PRINTI("[Socket:]", "Receiving package failed.");
		} else
		{
			PRINTI("[Socket:]", "Received message of length %d\n", length);
		}
		return length;
	}

	//	int Socket::Receive(OUT void* buffer, util::size_t size_in_bytes, OUT ipv4_t* ip)
	//	{
	//		sockaddr_in sockaddr_in;
	//		socklen_t socketAddressSize = sizeof(sockaddr_in);
	//		
	//		auto length = recvfrom(_id, buffer, size_in_bytes, 0, reinterpret_cast<sockaddr*>(&sockaddr_in), IN &socketAddressSize);
	//		if(length < 0)
	//		{
	//			PRINTI("[Socket:]", "Receiving package failed.");
	//		}
	//		else
	//		{
	//			PRINTI("[Socket:]", "Received message of length %d\n", length);
	//		}
	//		*ip = sockaddr_in.sin_addr.s_addr;
	//		return length;
	//	}

	bool Socket::IsTCP() const
	{
		return _protocol == SOCK_STREAM;
	}

	//	void Socket::Connect(const char* ipv4)
	//	{
	//		if(IsTCP()) // TCP
	//		{
	//			_address.sin_addr.s_addr = inet_addr(ipv4);
	//			connect(_id, (sockaddr *)&_address, sizeof(_address));
	//			PRINTI("[Socket:]", "Connected to address '%s'.\n", ipv4);
	//			return;
	//		}
	//		// else UDP
	//		if(ipv4[0] == '\0')
	//		{
	//			_address.sin_addr.s_addr = INADDR_ANY;
	//			int bc = 1;
	//			if(setsockopt(_id, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(bc)) < 0)
	//			{
	//				PRINTI("[Socket:]", "Failed to set socket option to broadcast.\n");
	//				closesocket(_id);
	//				return;
	//			}
	//			//bind(_id, (sockaddr*)&_address, sizeof(_address));
	//			PRINTI("[Socket:]", "Connected to any address.\n");
	//		}
	//		else
	//		{
	//			_address.sin_addr.s_addr = inet_addr(ipv4);
	//			PRINTI("[Socket:]", "Connected to address '%s'.\n", ipv4);
	//		}
	//	}

	SocketError Socket::Connect(ipv4_t ip)
	{
		if(IsTCP()) // TCP
		{
			_address.sin_addr.s_addr = ip;
			int enable = 1;
			setsockopt(_id, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
			int err = connect(_id, (sockaddr*)&_address, sizeof(_address));
			if(err == 0)
			{
				PRINTI("[Socket:]", "Connected to ip '%lu'\n", ip);
				return SocketError::NO_ERROR;
			}

			//in_addr addr;
			//addr.s_addr = htonl(ip); // Convert to network byte order
			//char ipAddressString[INET_ADDRSTRLEN];
			//inet_ntop(AF_INET, &addr, ipAddressString, INET_ADDRSTRLEN);

			PRINTI("[Socket:]", "Unable to connect with msg: '%s'\n", strerror(errno));
			return SocketError::CONNECTING_FAILED;
		}
		// else UDP
		_address.sin_addr.s_addr = ip ? ip : INADDR_ANY;
		int err = bind(_id, (sockaddr*)&_address, sizeof(_address));
		if(err == 0)
		{
			PRINTI("[Socket:]", "Connected to ip '%lu'\n", ip);
			return SocketError::NO_ERROR;
		}
		PRINTI("[Socket:]", "Unable to connect with msg: '%s'\n", strerror(errno));
		return SocketError::CONNECTING_FAILED;
	}

	bool Socket::IsConnected()
	{
		return _address.sin_addr.s_addr != 0;
	}

	void Socket::SetTimeout(long seconds, long microseconds)
	{
		timeval t
		{
			.tv_sec  = seconds,
			.tv_usec = microseconds,
		};
		setsockopt(_id, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
	}
}
