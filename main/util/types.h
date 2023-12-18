#pragma once

#include <cstdint>

namespace util
{
	using byte = unsigned char;
	using timestamp_t = long long;
	using size_t = std::uint32_t;

	constexpr byte PADDING_BYTE = 0x00;
}

using ascii_t = char;
