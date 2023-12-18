#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_freertos_hooks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>
#include <cstdio>

#include "tasks/sensor_control.h"
#include "tasks/transmitter_task.h"
#include "memory/ring_buffer.h"

constexpr static uint32_t SENSOR_CONTROL_TASK_STACK_SIZE        = 40'000;
constexpr static uint32_t TELEMETRY_TRANSMITTER_TASK_STACK_SIZE = 10'000;

TaskHandle_t SensorControl = nullptr;
TaskHandle_t TelemetryTransmitter = nullptr;

extern "C"
void app_main(void) 
{
    //mem::RingBufferView buffers;
    mem::RingBufferView view;

    BaseType_t result;
    result = xTaskCreatePinnedToCore(
        sys::sensor_control_task, 
        "SensorControlTask", 
        SENSOR_CONTROL_TASK_STACK_SIZE, 
        &view,
        30,
        &SensorControl,
        1
    );

    assert(result == pdPASS && "[SensorControlTask:] **Fatal** Could not allocate required memory!");

    while(view.size() == 0) { /* Do nothing */ } // Sensor Controller Task has to write  to tmp if ready

    PRINTI("[SensorControlTask:]", "Task created successfully!\n");

    //vTaskPrioritySet(xTaskGetCurrentTaskHandle(), 19);
    //sys::transmitter_task(&view);

    result = xTaskCreatePinnedToCore(
        sys::transmitter_task,
        "TelemetryTransmitterTask",
        TELEMETRY_TRANSMITTER_TASK_STACK_SIZE,
        &view,
        24,
        &TelemetryTransmitter,
        0
    );
    
    while(view.size()) { /* Do nothing */ }
}
