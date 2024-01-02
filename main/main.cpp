#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_freertos_hooks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>

#include "tasks/sensor_control.h"
#include "tasks/transmitter_task.h"
#include "memory/ring_buffer.h"
#include "config/task.h"

namespace config
{
    TaskHandle_t SensorControl = nullptr;
    TaskHandle_t TelemetryTransmitter = nullptr;
}

extern "C"
void app_main(void) 
{
    //mem::RingBufferView buffers;
    mem::RingBufferView view;

    BaseType_t result;

#if PIN_SENSOR_CONTROL
    result = xTaskCreatePinnedToCore(
#else
    result = xTaskCreate(
#endif
        sys::sensor_control_task, 
        "SensorControlTask", 
        config::SENSOR_CONTROL_TASK_STACK_SIZE,
        &view,
        config::SENSOR_CONTROL_TASK_PRIORITY,
        &config::SensorControl
#if PIN_SENSOR_CONTROL
        ,config::SENSOR_CONTROL_TASK_CORE
#endif
    );

    assert(result == pdPASS && "[SensorControlTask:] **Fatal** Could not allocate required memory!");

    while(view.size() == 0) { /* Do nothing */ } // Sensor Controller Task has to write  to tmp if ready

    PRINTI("[SensorControlTask:]", "Task created successfully!\n");

    //vTaskPrioritySet(xTaskGetCurrentTaskHandle(), 19);
    //sys::transmitter_task(&view);

#if PIN_TELEMETRY_TRANSMITTER
    result = xTaskCreatePinnedToCore(
#else
    result = xTaskCreate(
#endif
        sys::transmitter_task,
        "TelemetryTransmitterTask",
        config::TELEMETRY_TRANSMITTER_TASK_STACK_SIZE,
        &view,
        config::TELEMETRY_TRANSMITTER_TASK_PRIORITY,
        &config::TelemetryTransmitter
#if PIN_TELEMETRY_TRANSMITTER
        ,config::TELEMETRY_TRANSMITTER_TASK_CORE
#endif
    );
    
    while(view.size()) { /* Do nothing */ }
}
