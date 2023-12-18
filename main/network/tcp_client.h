#pragma once

#include <array>
#include <cstring>

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
		void WaitFor(std::array<char, SIZE> const& value) const
		{
			char buffer[SIZE];
			do
			{
				DISCARD Receive(buffer, SIZE);
			}
			while (std::memcmp(buffer, value.data(), SIZE));
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
