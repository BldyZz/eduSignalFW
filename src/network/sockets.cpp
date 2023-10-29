#include "sockets.h"

#include <cstdint>
#include <string_view>
#include <sys/socket.h>
#include <fmt/format.h>
#include <netdb.h>

#define OUT
#define IN

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
			fmt::print("[UDP:] Failed to set socket options: errno = {}\n", strerror(errno));
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
			fmt::print("[UDP:] Failed to bind socket.\n");
			return INVALID_CLIENT;
		}

		// Receive
		fmt::print("[UDP:] Successfully bound socket. Wait for message on port '{}'...\n", PORT);
		sockaddr_in sockaddr_in;
		socklen_t socketAddressSize = sizeof(sockaddr_in);

		int isCmd = false;
		do
		{
			int length = recvfrom(socket, buffer, std::size(cmd) - 1, 0, OUT reinterpret_cast<sockaddr*>(&sockaddr_in), IN & socketAddressSize);
			buffer[std::size(cmd) - 1] = '\0';
			isCmd = strcmp(buffer, cmd);
			fmt::print("{}, {}\n", buffer, isCmd);
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
			fmt::print("[UDP:] Failed to set socket options: errno = {}\n", strerror(errno));
			close_udp_socket(socket);
			return;
		}
		
		sockaddr_in receiverAddress;
		receiverAddress.sin_family      = AF_INET;
		receiverAddress.sin_port        = htons(PORT);
		receiverAddress.sin_addr.s_addr = client.ipv4;
		
		if(bind(socket, reinterpret_cast<sockaddr*>(&receiverAddress), sizeof(receiverAddress)) < 0)
		{
			fmt::print("[UDP:] Failed to bind socket.\n");
			return;
		}

		ssize_t length = sendto(socket, OK, std::size(OK), 0, reinterpret_cast<sockaddr*>(&receiverAddress), sizeof(receiverAddress));
		fmt::print("[UDP:] Send message to \"{}\".\n", client.ipv4);
	}
}
