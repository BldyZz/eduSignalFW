#pragma once

#include <cstdint>

namespace mem
{
	/**
	 * \brief For storage purposes only!: 24-Bit signed integer type.
	 */
	struct int24_t
	{
		using owner_type = int24_t;
		using owner_ref  = owner_type&;
		using owner_cref = owner_type const&;

		uint8_t _value[3];
		// u = unsigned, s = signed, ms = most significant, ls = least significant
		
		int24_t() = default;

		int24_t(uint32_t const& value);
		int24_t(int32_t const& value);
		int24_t(int16_t const& value);
		int24_t(uint16_t const& value);

		// Copy
		int24_t(int24_t const& other);
		int24_t& operator=(int24_t const& other);

		// Operators
		int24_t& operator=(uint32_t const& value);
		int24_t& operator=(int32_t const& value);
		int24_t& operator=(int16_t const& value);
		int24_t& operator=(uint16_t const& value);
	};
}
