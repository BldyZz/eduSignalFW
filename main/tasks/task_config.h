#pragma once

#define WATCHDOG_TIME_FOR_HANDLING 10
#define YIELD_FOR(ms)              xTaskNotifyWait(0, 0, nullptr, pdMS_TO_TICKS(ms))
#define WATCHDOG_HANDLING()        YIELD_FOR(WATCHDOG_TIME_FOR_HANDLING) 