#include "sockets.h"

#include <cstdio>
#include <cstdint>
#include <string_view>
#include <sys/socket.h>
#include <netdb.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "../util/defines.h"

namespace net
{
	bool operator==(client_t const& left, client_t const& right)
	{
		return left.ipv4 == right.ipv4 && left.port == right.port;
	}

	udp_socket open_udp_socket()
	{
		return socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	}

	void close_udp_socket(udp_socket const& socket)
	{
		shutdown(socket, 0);
		close(socket);
	}

	bool is_valid(udp_socket const& socket)
	{
		return socket >= 0;
	}

	bool is_valid(client_t const& client)
	{
		return client != INVALID_CLIENT;
	}

	client_t check_for_clients(udp_socket socket)
	{
		static constexpr uint16_t PORT = 1212;
		static constexpr char cmd[] = "EDF-DISCOVER";
		char buffer[std::size(cmd)];

		if(setsockopt(socket, SOL_SOCKET, SO_RCVBUF, buffer, std::size(buffer)))
		{
			PRINTI("[UDP:]", "Failed to set socket options: \"%s\" \n", strerror(errno));
			close_udp_socket(socket);
			return INVALID_CLIENT;
		}

		// Bind Socket
		sockaddr_in receiverAddress;
		receiverAddress.sin_family      = AF_INET;
		receiverAddress.sin_port        = htons(PORT);
		receiverAddress.sin_addr.s_addr = INADDR_ANY;

		if(bind(socket, (sockaddr*)&receiverAddress, sizeof(receiverAddress)) < 0)
		{
			PRINTI("[UDP:]", "Failed to bind socket.\n");
			return INVALID_CLIENT;
		}

		// Receive
		PRINTI("[UDP:]", "Successfully bound socket. Wait for message on port '%u'...\n", PORT);
		sockaddr_in sockaddr_in;
		socklen_t socketAddressSize = sizeof(sockaddr_in);

		int isCmd = false;
		do
		{
			(void)recvfrom(socket, buffer, std::size(cmd) - 1, 0, OUT reinterpret_cast<sockaddr*>(&sockaddr_in), IN & socketAddressSize);
			buffer[std::size(cmd) - 1] = '\0';
			isCmd = strcmp(buffer, cmd);
			//PRINTI("[UDP:]", "%s\n", buffer);
		} while(isCmd);
		
		return client_t{.ipv4 = sockaddr_in.sin_addr.s_addr, .port = PORT};
	}

	void send_ok(udp_socket socket, const client_t& client)
	{
		static constexpr uint16_t PORT = 1212;
		constexpr char OK[] = "OK";

		timeval timeout;
		timeout.tv_sec = 1000;
		timeout.tv_usec = 0;

		if(setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)))
		{
			PRINTI("[UDP:]", "Failed to set socket options: \"%s\"\n", strerror(errno));
			close_udp_socket(socket);
			return;
		}
		
		sockaddr_in receiverAddress;
		receiverAddress.sin_family      = AF_INET;
		receiverAddress.sin_port        = htons(PORT);
		receiverAddress.sin_addr.s_addr = client.ipv4;
		
		if(bind(socket, reinterpret_cast<sockaddr*>(&receiverAddress), sizeof(receiverAddress)) < 0)
		{
			PRINTI("[UDP:]", "Failed to bind socket.\n");
			return;
		}

		(void)sendto(socket, OK, std::size(OK), 0, reinterpret_cast<sockaddr*>(&receiverAddress), sizeof(receiverAddress));
		PRINTI("[UDP:]", "Send message to \"%lu\".\n", client.ipv4);

		// TODO: OK send
	}

	void send_pkg(udp_socket socket, client_t const& client, void* data, util::size_t size_in_bytes)
	{
		static constexpr uint16_t PORT = 1212;

		timeval timeout;
		timeout.tv_sec = 1000;
		timeout.tv_usec = 0;

		if(setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)))
		{
			PRINTI("[UDP:]", "Failed to set socket options: \"%s\"\n", strerror(errno));
			close_udp_socket(socket);
			return;
		}

		sockaddr_in receiverAddress;
		receiverAddress.sin_family = AF_INET;
		receiverAddress.sin_port = htons(PORT);
		receiverAddress.sin_addr.s_addr = client.ipv4;

		if(bind(socket, reinterpret_cast<sockaddr*>(&receiverAddress), sizeof(receiverAddress)) < 0)
		{
			PRINTI("[UDP:]", "Failed to bind socket.\n");
			return;
		}

		(void)sendto(socket, data, size_in_bytes, 0, reinterpret_cast<sockaddr*>(&receiverAddress), sizeof(receiverAddress));
	}
}