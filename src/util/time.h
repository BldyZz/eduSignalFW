#pragma once

#include <chrono>

#include "types.h"

namespace util
{
	class Time
	{
	public:
		Time();

		timestamp_t GetTimestamp() const;
	private:
		using clock_t     = std::chrono::high_resolution_clock;
		using timepoint_t = std::chrono::time_point<clock_t>;
		using unit_t      = std::chrono::milliseconds;

		timepoint_t _begin;
	};

	extern Time gTime;
}	