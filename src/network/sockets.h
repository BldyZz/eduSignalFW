#pragma once

#include <array>
#include <cstdint>
#include <cstring>

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


	class Socket
	{
	public:
		Socket();

		void Open(Protocol protocol, port_t port, Domain domain = Domain::IPv4);
		void Close();

		template<size_t Size>
		void AutoConnect(std::array<char, Size> const& compareBuffer)
		{
			char buffer[Size];
			AutoConnect(compareBuffer.data(), Size, buffer);
		}

		void AutoConnect(const char* compareBuffer, util::size_t size_in_bytes, OUT char* buffer); // Receives a broadcast and connects to the ip of the first package which has the same data like the compareBuffer on the sockets port.
		void Connect(const char* ipv4);
		void Connect(ipv4_t ip);
		bool IsConnected();

		template<typename T, size_t Size>
		void Send(std::array<T, Size> const& buffer)
		{
			Send(buffer.data(), Size * sizeof(T));
		}
		void Send(void const* data, util::size_t size_in_bytes);

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
		int Receive(OUT void* buffer, util::size_t size_in_bytes, ipv4_t* ip);

		void SetTimeout(long seconds, long microseconds);
	private:

		bool IsTCP() const;

		union
		{
			sockaddr_in  _address;
			//sockaddr_in6 address6;
		};
		int32_t _id;	   // Socket ID
		int32_t _protocol; // UDP, TCP, ...?
		port_t  _port;
	};
}
