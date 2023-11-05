#include "int.h"

namespace mem
{
	uint24_t::uint24_t()
	{
	}

	uint24_t::uint24_t(uint24_t const& other)
	{
		u_ms_16 = other.u_ms_16;
		u_ls_8  = other.u_ls_8;
	}

	uint24_t& uint24_t::operator=(uint24_t const& other)
	{
		u_ms_16 = other.u_ms_16;
		u_ls_8  = other.u_ls_8;
		return *this;
	}

	uint24_t::uint24_t(uint32_t value)
	{
		std::memcpy(_value, &value, util::total_size(_value));
	}

	uint24_t& uint24_t::operator=(uint32_t value)
	{
		std::memcpy(_value, &value, util::total_size(_value));
		return *this;
	}



	int24_t::int24_t()
	{
	}

	int24_t::int24_t(const int24_t& other)
	{
		u_ms_16 = other.u_ms_16;
		u_ls_8  = other.u_ls_8; 
	}

	int24_t& int24_t::operator=(int24_t const& other)
	{
		u_ms_16 = other.u_ms_16;
		u_ls_8  = other.u_ls_8; 
		return *this;
	}

	int24_t::int24_t(int32_t value)
	{
		std::memcpy(_value, &value, util::total_size(_value)); // truncation
	}

	int24_t::int24_t(const int16_t& value)
	{
		s_ls_16 = value;
		u_ms_8  = value & 0x8000 ? 0xFF : 0x00;
	}

	int24_t::int24_t(const int8_t& value)
	{
		u_ms_16 = value & 0x80 ? 0xFFFF : 0x0000;
		s_ls_8 = value;
	}

	int24_t::int24_t(const uint8_t& value)
	{
		u_ms_16 = 0x0000;
		u_ls_8  = value;
	}

	int24_t& int24_t::operator=(uint32_t value)
	{
		value &= 0x0007'FFFF;
		std::memcpy(_value, &value, util::total_size(_value));
		return *this;
	}
	int24_t& int24_t::operator=(const uint16_t& value)
	{
		u_ls_16 = value;
		u_ms_8  = 0;
		return *this;
	}

	int24_t& int24_t::operator=(const int8_t& value)
	{
		u_ms_16 = value & 0x80 ? 0xFFFF : 0x0000;
		s_ls_8  = value;
		return *this;
	}

	int24_t& int24_t::operator=(const uint8_t& value)
	{
		u_ms_16 = 0x0000;
		u_ls_8  = value;
		return *this;
	}
}
