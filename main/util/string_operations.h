#pragma once
#include <array>

using ascii_t = char;

namespace util
{
	template<size_t Size>
	constexpr auto non_terminated(const ascii_t(&str)[Size]) -> std::array<ascii_t, Size - 1>
	{
		static_assert(Size > 1, "non_terminated: The size of the given string is 1. The return value can't be of size 0.");
		std::array<ascii_t, Size - 1> ret;
		for(size_t i = 0; i < Size - 1; i++)
		{
			ret[i] = str[i];
		}
		return ret;
	}
}
