#pragma once

#include "types.h"
#include "defines.h"

namespace util::literals
{
	b24_t operator ""_24(int32_t value)
	{
		return b24_t{ value >> BYTES_TO_BITS(1)};
	}
}
