#include "int.h"

#include "../util/defines.h"

namespace mem
{
	uint24_t::uint24_t(uint24_t const& other)
	{
		operator=(other);
	}

	uint24_t& uint24_t::operator=(uint24_t const& other)
	{
		_value[0] = other._value[0];
		_value[1] = other._value[1];
		_value[2] = other._value[2];
		return *this;
	}

	uint24_t::uint24_t(uint32_t value)
	{
		operator=(value);
	}

	uint24_t& uint24_t::operator=(uint32_t value)
	{
		_value[0] = value & 0xFF;
		_value[1] = (value & 0xFF00) >> BYTES_TO_BITS(1);
		_value[2] = (value & 0xFF0000) >> BYTES_TO_BITS(2);
		return *this;
	}








	int24_t::int24_t(const int24_t& other)
	{
		operator=(other);
	}

	int24_t& int24_t::operator=(int24_t const& other)
	{
		_value[0] = other._value[0];
		_value[1] = other._value[1];
		_value[2] = other._value[2];
		return *this;
	}

	int24_t::int24_t(int32_t value)
	{
		_value[0] = value & 0xFF;
		_value[1] = (value & 0xFF00) >> BYTES_TO_BITS(1);
		_value[2] = (value & 0xFF0000) >> BYTES_TO_BITS(2);
	}

	int24_t::int24_t(const int16_t& value)
	{
		_value[0] = value & 0xFF;
		_value[1] = (value & 0xFF00) >> BYTES_TO_BITS(1);
		_value[2] = value < 0 ? 0xFF : 0x00;
	}

	int24_t::int24_t(const int8_t& value)
	{
		operator=(value);
	}

	int24_t::int24_t(const uint8_t& value)
	{
		operator=(value);
	}

	int24_t& int24_t::operator=(uint32_t value)
	{
		_value[0] = value & 0xFF;
		_value[1] = (value & 0xFF00) >> BYTES_TO_BITS(1);
		_value[2] = (value & 0x7F0000) >> BYTES_TO_BITS(2);
		return *this;
	}
	int24_t& int24_t::operator=(const uint16_t& value)
	{
		_value[0] = value & 0xFF;
		_value[1] = value & 0xFF >> BYTES_TO_BITS(1);
		_value[2] = 0;
		return *this;
	}

	int24_t& int24_t::operator=(const int8_t& value)
	{
		_value[0] = value & 0xFF;
		_value[1] = value < 0 ? 0xFF : 0x00;
		_value[2] = value < 0 ? 0xFF : 0x00;
		return *this;
	}

	int24_t& int24_t::operator=(const uint8_t& value)
	{
		_value[0] = value;
		_value[1] = 0;
		_value[2] = 0;
		return *this;
	}
}
