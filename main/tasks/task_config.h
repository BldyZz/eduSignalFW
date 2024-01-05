#pragma once

#define NO_DELAY      0
#define YIELD_FOR(ms) vTaskDelay(pdMS_TO_TICKS(ms))
