#include "esp_system.h"
#include "esp_task_wdt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

#include "esp_util/i2cDevice.hpp"
#include <thread>

#include <cstdint>
#include <fmt/chrono.h>
#include <fmt/format.h>

#include "tasks/sensor_control.h"
#include "tasks/telemetry_transmitter.h"

constexpr static uint32_t SENSOR_CONTROL_TASK_STACK_SIZE = 40'000;
constexpr static uint32_t TELEMETRY_TRANSMITTER_TASK_STACK_SIZE = 10'000;


TaskHandle_t SensorControl = nullptr;
TaskHandle_t TelemetryTransmitter = nullptr;

extern "C"
void app_main() 
{
    struct
    {
        mem::ring_buffer_t** sensorDataSource = nullptr;
        size_t               ringBufferCount  = 0;
    } tmp;

    static_assert(sizeof(tmp) == 8); // Assert that the system is 32-Bit.

    BaseType_t result;
    result = xTaskCreate(
        sys::sensor_control_task, 
        "SensorControlTask", 
        SENSOR_CONTROL_TASK_STACK_SIZE, 
        &tmp.sensorDataSource, 
        tskIDLE_PRIORITY,
        &SensorControl
    );

    if(result != pdPASS)
    {
        fmt::print("[SensorControlTask:] **Fatal** Could not allocate required memory!");
        return;
    }

    while(!tmp.ringBufferCount) { /* Do nothing */ } // Sensor Controller Task has to write  to tmp if ready
    
    fmt::print("[SensorControlTask:] Task created successfully!\n");

    sys::telemetry_transmitter_task(&tmp.sensorDataSource);

    //result = xTaskCreate(
    //    sys::telemetry_transmitter_task,
    //    "TelemetryTransmitterTask",
    //    TELEMETRY_TRANSMITTER_TASK_STACK_SIZE,
    //    &tmp.sensorDataSource,
    //    tskIDLE_PRIORITY,
    //    &TelemetryTransmitter
    //);
    //
    //if(result != pdPASS)
    //{
    //    fmt::print("[TelemetryTransmitterTask:] **Fatal** Could not allocate required memory!\n");
    //    vTaskDelete(SensorControl);
    //    return;
    //}
    //
    //while(tmp.sensorDataSource) { /* Do nothing */ }; // Telemetry task has to write nullptr into tmp if ready
    //fmt::print("[TelemetryTransmitterTask:] Task created successfully!\n");

}
