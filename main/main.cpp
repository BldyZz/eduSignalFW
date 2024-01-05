#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_freertos_hooks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>
#include <cstdio>

#include "tasks/sensor_control.h"
#include "tasks/transmitter_task.h"
#include "memory/sensor_data.h"
#include "memory/int.h"

constexpr static uint32_t SENSOR_CONTROL_TASK_STACK_SIZE        = 40'000;
constexpr static uint32_t TELEMETRY_TRANSMITTER_TASK_STACK_SIZE = 10'000;

TaskHandle_t SensorControl = nullptr;
TaskHandle_t TelemetryTransmitter = nullptr;

extern "C"
void app_main(void) 
{
    BaseType_t result;
    result = xTaskCreatePinnedToCore(
        sys::sensor_control_task, 
        "SensorControlTask", 
        SENSOR_CONTROL_TASK_STACK_SIZE, 
        nullptr,
        26,
        &SensorControl,
        1
    );

    assert(result == pdPASS && "[SensorControlTask:] **Fatal** Could not allocate required memory!");
    PRINTI("[SensorControlTask:]", "Task created successfully!\n");

    result = xTaskCreatePinnedToCore(
        sys::transmitter_task,
        "TelemetryTransmitterTask",
        TELEMETRY_TRANSMITTER_TASK_STACK_SIZE,
        nullptr,
        24,
        &TelemetryTransmitter,
        0
    );
}
