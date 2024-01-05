#pragma once

#include <array>
#include <cstring>
#include <cstdio>

#include "common.h"
#include "../util/defines.h"
#include "esp_attr.h"

namespace net
{
	enum class TCPError
	{
		NO_ERROR = 0,
		UNABLE_TO_OPEN_SOCKET,
		CONNECTING_FAILED,
		SENDING_FAILED,
		SOCKET_CLOSED,
		RECEIVING_FAILED,
	};

	class TCPClient
	{
	public:
		TCPClient(port_t port);
		~TCPClient();

		TCPError Open();

		TCPError Connect(const char* ip); // Opens a client for a specific target
		void Close();

		TCPError IRAM_ATTR Send(void const* data, size_t size_in_bytes);
		int  Receive(OUT void* data, size_t size_in_bytes) const;
		template<size_t SIZE>
		NODISCARD TCPError WaitFor(std::array<char, SIZE> const& value) const
		{
			char buffer[SIZE];
			do
			{
				const int received = Receive(buffer, SIZE);
				if(received <= 0)
				{
					if(received == 0)
					{
						PRINTI("[TCP:]", "Cannot receive TCP message. Socket is already closed.\n");
						return TCPError::SOCKET_CLOSED;
					}
					return TCPError::RECEIVING_FAILED;
				}
			}
			while (std::memcmp(buffer, value.data(), SIZE));
			return TCPError::NO_ERROR;
		}
		template<size_t SIZE>
		bool Check(std::array<char, SIZE> const& value) const
		{
			char      buffer[SIZE];
			const int len = Receive(buffer, SIZE);
			if(len < 0) return false;
			return !std::memcmp(buffer, value.data(), SIZE);
		}

		void SetTimeout(long const& s, long const& us);
		bool IsConnected();

	private:

		socket_id _id;
		port_t    _port;
	};
}
