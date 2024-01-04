#include "int.h"

#include "../util/defines.h"

namespace mem
{

	int24_t::int24_t(uint32_t const& value)
	{
		DISCARD operator=(value);
	}

	int24_t::int24_t(int32_t const& value)
	{
		DISCARD operator=(value);
	}

	int24_t::int24_t(int16_t const& value)
	{
		DISCARD operator=(value);
	}

	int24_t::int24_t(uint16_t const& value)
	{
		DISCARD operator=(value);
	}

	auto int24_t::operator=(uint32_t const& value) -> owner_ref
	{
		_value[0] = value & 0xFF;
		_value[1] = (value & 0xFF00) >> BYTES_TO_BITS(1);
		_value[2] = (value & 0x7F0000) >> BYTES_TO_BITS(2);
		return *this;
	}

	auto int24_t::operator=(int32_t const& value) -> owner_ref
	{
		_value[0] = value & 0xFF;
		_value[1] = (value & 0xFF00) >> BYTES_TO_BITS(1);
		_value[2] = (value & 0xFF0000) >> BYTES_TO_BITS(2);
		return *this;
	}

	auto int24_t::operator=(int16_t const& value) -> owner_ref
	{
		_value[0] = value & 0xFF;
		_value[1] = (value & 0xFF00) >> BYTES_TO_BITS(1);
		_value[2] = value < 0 ? 0xFF : 0x00;
		return *this;
	}

	auto int24_t::operator=(const uint16_t& value) -> owner_ref
	{
		_value[0] = value & 0xFF;
		_value[1] = value & 0xFF >> BYTES_TO_BITS(1);
		_value[2] = 0;
		return *this;
	}
	
}
