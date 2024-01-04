#pragma once

#define WATCHDOG_TIME_FOR_HANDLING 10
#define YIELD_FOR(ms)              vTaskDelay(pdMS_TO_TICKS(ms))
#define WATCHDOG_HANDLING()        YIELD_FOR(WATCHDOG_TIME_FOR_HANDLING)

#include "../memory/sensor_data.h"

namespace mem
{
	struct int24_t;
}

