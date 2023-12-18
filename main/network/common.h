#pragma once

#include <cstdint>

namespace net
{
	using ipv4_t = uint32_t;
	using port_t = int32_t;
	using socket_id = int32_t;
	using ipv6_t = char[16];

	constexpr bool USE_IPV4 = true;
}
