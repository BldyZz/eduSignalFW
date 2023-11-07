#pragma once

#include <cstdint>

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

		void AutoConnect(); // Receives a broadcast and connects to the ip of the first package on the sockets port.
		void Connect(const char* ipv4);
		void Connect(ipv4_t ip);

		void Send(void* data, util::size_t size_in_bytes);
		void Receive(OUT void* buffer, util::size_t size_in_bytes);
		void Receive(OUT void* buffer, util::size_t size_in_bytes, ipv4_t* ip);

		void SetTimeout(long seconds);
	private:

		bool IsTCP() const;

		union
		{
			sockaddr_in  _address;
			//sockaddr_in6 address6;
		};
		int32_t _id;		  // Socket ID
		int32_t _protocol; // UDP, TCP, ...?
		port_t  _port;
	};
}
