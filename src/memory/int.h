#pragma once

#include <cstdint>

namespace mem
{
	/**
	 * \brief For storage purposes only!: 24-Bit unsigned integer type. 
	 */
	struct uint24_t
	{
		uint8_t _value[3];

		uint24_t() = default;

		uint24_t(uint24_t const& other);
		uint24_t& operator=(uint24_t const& other);

		uint24_t(uint24_t&&)            = delete;
		uint24_t& operator=(uint24_t&&) = delete;

		uint24_t(uint32_t value);
		uint24_t& operator=(uint32_t value);
	};

	/**
	 * \brief For storage purposes only!: 24-Bit integer type.
	 */
	struct int24_t
	{
		uint8_t _value[3];
		// u = unsigned, s = signed, ms = most significant, ls = least significant

		int24_t() = default;

		// Copy
		explicit int24_t(const int24_t& other);
		int24_t& operator=(const int24_t& other);

		explicit int24_t(int32_t value);
		explicit int24_t(const int16_t& value);
		explicit int24_t(const int8_t& value);
		explicit int24_t(const uint8_t& value);
		
		int24_t& operator=(uint32_t value);
		int24_t& operator=(const uint16_t& value);
		int24_t& operator=(const int8_t& value);
		int24_t& operator=(const uint8_t& value);
	};
}
