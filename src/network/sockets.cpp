#include "sockets.h"

#include <cassert>
#include <cstdio>
#include <cstdint>
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
		shutdown(_id, 0);
		close(_id);
	}

	void Socket::Send(void* data, util::size_t size_in_bytes)
	{
		auto err = send(_id, data, size_in_bytes, 0);
		if(err < 0) 
		{
			PRINTI("[Socket:]", "Sending package failed.");
		}
	}

	void Socket::Receive(OUT void* buffer, util::size_t size_in_bytes)
	{
		auto err = recv(_id, buffer, size_in_bytes - 1, 0);
		if(err < 0)
		{
			PRINTI("[Socket:]", "Receiving package failed.");
		}
	}

	void Socket::Receive(OUT void* buffer, util::size_t size_in_bytes, ipv4_t* ip)
	{
		sockaddr_in sockaddr_in;
		socklen_t socketAddressSize = sizeof(sockaddr_in);

		auto err = recvfrom(_id, buffer, size_in_bytes - 1, 0, reinterpret_cast<sockaddr*>(&sockaddr_in), IN & socketAddressSize);
		if(err < 0)
		{
			PRINTI("[Socket:]", "Receiving package failed.");
		}
		*ip = sockaddr_in.sin_addr.s_addr;
	}

	void Socket::AutoConnect()
	{
		get_ip_info();

		static constexpr char cmd[] = "BDF-DISCOVER";
		ipv4_t ip;

		Socket udp;
		udp.Open(Protocol::UDP, _port);
		udp.Connect("");

		char buf[util::total_size(cmd)];

		do
		{
			udp.Receive(buf, util::total_size(cmd), OUT &ip);
			PRINTI("[AutoConnect:]", "Msg\n");
		} while(std::strcmp(cmd, buf) != 0);
		
		udp.Close();

		Connect(ip);
	}

	bool Socket::IsTCP() const
	{
		return _protocol == SOCK_STREAM;
	}

	void Socket::Connect(const char* ipv4)
	{
		if(IsTCP()) // TCP
		{
			_address.sin_addr.s_addr = inet_addr(ipv4);
			connect(_id, (sockaddr *)&_address, sizeof(_address));
			return;
		}
		// else UDP
		if(ipv4[0] == '\0')
		{
			_address.sin_addr.s_addr = INADDR_ANY;
		}
		else
		{
			_address.sin_addr.s_addr = inet_addr(ipv4);
		}
	}

	void Socket::Connect(ipv4_t ip)
	{
		if(!IsTCP()) // TCP
		{
			_address.sin_addr.s_addr = ip;
			connect(_id, (sockaddr*)&_address, sizeof(_address));
			return;
		}
		// else UDP
		if(!ip) 
		{
			_address.sin_addr.s_addr = INADDR_ANY;
		}
		else
		{
			_address.sin_addr.s_addr = ip;
		}
	}

	void Socket::SetTimeout(long seconds)
	{
		timeval t
		{
			.tv_sec  = seconds,
			.tv_usec = 0
		};
		setsockopt(_id, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
	}
}
