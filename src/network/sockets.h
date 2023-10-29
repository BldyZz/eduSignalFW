#pragma once

#include <cstdint>

#include "../util/defines.h"

namespace net
{
	using ipv4_t     = uint32_t;
	using port_t     = int32_t;
	using udp_socket = int32_t;
	struct client_t
	{
		ipv4_t ipv4;
		port_t port;
	};

	constexpr ipv4_t   INVALID_IP = 0;
	constexpr port_t   INVALID_PORT   = 2030;
	constexpr client_t INVALID_CLIENT = 
	{
		.ipv4   = INVALID_IP,
		.port = INVALID_PORT,
	};

	NODISCARD bool operator==(client_t const& left, client_t const& right);

	CHECKVAL NODISCARD udp_socket open_udp_socket();
	void close_udp_socket(udp_socket const& socket);
	NODISCARD bool is_valid(udp_socket const& socket);
	NODISCARD bool is_valid(client_t const& client);

	CHECKVAL NODISCARD client_t check_for_clients(udp_socket socket);
	void      send_ok(udp_socket socket, const client_t& client);
}
