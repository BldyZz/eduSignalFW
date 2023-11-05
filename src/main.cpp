#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_freertos_hooks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>
#include <cstdio>

#include "tasks/sensor_control.h"
#include "tasks/telemetry_transmitter.h"
#include "memory/ring_buffer.h"
#include "memory/nvs.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

constexpr static uint32_t SENSOR_CONTROL_TASK_STACK_SIZE = 40'000;
constexpr static uint32_t TELEMETRY_TRANSMITTER_TASK_STACK_SIZE = 10'000;



TaskHandle_t SensorControl = nullptr;
TaskHandle_t TelemetryTransmitter = nullptr;

extern "C"
void app_main() 
{
    esp_util::nvs_init(); 
    mem::RingBufferArray buffers;

    BaseType_t result;
    result = xTaskCreate(
        sys::sensor_control_task, 
        "SensorControlTask", 
        SENSOR_CONTROL_TASK_STACK_SIZE, 
        &buffers, 
        tskIDLE_PRIORITY,
        &SensorControl
    );

    assert(result == pdPASS && "[SensorControlTask:] **Fatal** Could not allocate required memory!");

    while(buffers.size == 0) { /* Do nothing */ } // Sensor Controller Task has to write  to tmp if ready

    ESP_LOGI("[SensorControlTask:]", "Task created successfully!");

    sys::telemetry_transmitter_task(&buffers);
        
    //result = xTaskCreate(
    //    sys::telemetry_transmitter_task,
    //    "TelemetryTransmitterTask",
    //    TELEMETRY_TRANSMITTER_TASK_STACK_SIZE,
    //    &tmp.sensorDataSource,
    //    tskIDLE_PRIORITY,
    //    &TelemetryTransmitter
    //);
    //
    //assert(result == pdPASS && "[TelemetryTransmitterTask:] **Fatal** Could not allocate required memory!");
    //
    //while(tmp.sensorDataSource) { /* Do nothing */ }; // Telemetry task has to write nullptr into tmp if ready
    //
    //printf("[TelemetryTransmitterTask:] Task created successfully!\n");
}
