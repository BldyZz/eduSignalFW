#pragma once

#include <cstdint>
#include <cstring>

#include "../util/utils.h"

namespace mem
{
	/**
	 * \brief For storage purposes only!: 24-Bit unsigned integer type. 
	 */
	union uint24_t
	{
		uint8_t _value[3];
		// u = unsigned, s = signed, ms = most significant, ls = least significant
		struct
		{
			uint16_t  u_ls_16;
			uint8_t   u_ms_8;
		};
		struct 
		{
			uint8_t  u_ls_8;
			uint16_t u_ms_16;
		};
		//msb_ubyte_t  u_bs;

		uint24_t();

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
	union int24_t
	{
		uint8_t _value[3];
		// u = unsigned, s = signed, ms = most significant, ls = least significant
		struct
		{
			uint16_t  u_ls_16;
			uint8_t   u_ms_8;
		};
		struct 
		{
			uint8_t  u_ls_8;
			uint16_t u_ms_16;
		};
		struct 
		{
			int16_t  s_ls_16;
			int8_t   s_ms_8;
		};
		struct 
		{
			int8_t   s_ls_8;
			int16_t  s_ms_16;
		};

		int24_t();
		~int24_t() = default;

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
