#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include "../util/defines.h"
#include "../util/types.h"


#include <sys/socket.h>

namespace net
{
	using ipv4_t     = uint32_t;
	
	
	using port_t     = int32_t;
	enum class Domain : util::byte
	{
		IPv4,
		//IPv6,
	};
	enum class Protocol : util::byte
	{
		UDP,
		TCP,
	};
	enum class SocketError
	{
		NO_ERROR = 0,
		CONNECTING_FAILED,
		SENDING_FAILED,
	};


	class Socket
	{
	public:
		Socket();

		void Open(Protocol protocol, port_t port, Domain domain = Domain::IPv4);
		void Close();

		void EstablishTCPConnection();

		template<size_t Size, size_t Size2>
		SocketError AutoConnect(std::array<char, Size> const& receiveCommand, std::array<char, Size2> const& sendCommand)
		{
			char buffer[Size];

			Socket udp;
			udp.Open(Protocol::UDP, _port);
			PRINTI("[Socket:]", "Opened port '%ld' for AutoConnect.\n", _port);
			udp.Connect();
			int cmp;
			do
			{
				udp.Receive(buffer, std::size(buffer));
				cmp = std::strncmp(receiveCommand.data(), buffer, std::size(buffer));
			}
			while(cmp != 0);
			PRINTI("[Socket:]", "Accepted auto connect message.\n");

			udp._address = udp._lastReceiveAddress;
			//if(udp.Connect(ip) == SocketError::CONNECTING_FAILED)
			//{
			//	return SocketError::CONNECTING_FAILED;
			//}
			if(udp.Send(sendCommand.data(), Size2) == SocketError::SENDING_FAILED)
			{
				return SocketError::SENDING_FAILED;
			}

			udp.Close();

			_address = udp._address;
			EstablishTCPConnection();
			return Connect(_address.sin_addr.s_addr);
		}

		//void Connect(const char* ipv4);
		SocketError Connect(ipv4_t ip = 0); // 0 = Any
		bool IsConnected();

		template<typename T, size_t Size>
		SocketError Send(std::array<T, Size> const& buffer)
		{
			return Send(buffer.data(), Size * sizeof(T));
		}
		SocketError Send(void const* data, util::size_t size_in_bytes);

		template<typename T, size_t Size>
		void ReceiveAndCompareIndefinite(std::array<T, Size> const& cmp)
		{
			while(!ReceiveAndCompare(cmp));
		}
		template<typename T, size_t Size>
		bool ReceiveAndCompare(std::array<T, Size> const& cmp)
		{
			T buffer[Size];
			DISCARD Receive(buffer, Size * sizeof(T));
			return !std::memcmp(buffer, cmp.data(), sizeof(T) * Size);
		}
		int Receive(OUT void* buffer, util::size_t size_in_bytes);
		//int Receive(OUT void* buffer, util::size_t size_in_bytes, OUT ipv4_t* ip);

		void SetTimeout(long seconds, long microseconds);
	private:

		bool IsTCP() const;

		union
		{
			sockaddr_in  _address;
			//sockaddr_in6 address6;
		};
		sockaddr_in _lastReceiveAddress;
		int32_t _id;	   // Socket ID
		int32_t _protocol; // UDP, TCP, ...?
		port_t  _port;
	};
}
