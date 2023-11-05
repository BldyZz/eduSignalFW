#include "time.h"

namespace util
{
	Time gTime = Time();

	Time::Time()
	{
		_begin = clock_t::now();
	}

	timestamp_t Time::GetTimestamp() const
	{
		return std::chrono::duration_cast<unit_t>(clock_t::now() - _begin).count();
	}

}
