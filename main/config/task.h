#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#define NO_DELAY 0
#define WATCHDOG_TIME_FOR_HANDLING 10
#define YIELD_FOR(ms)              xTaskNotifyWait(0, 0, nullptr, pdMS_TO_TICKS(ms))
#define WATCHDOG_HANDLING()        YIELD_FOR(WATCHDOG_TIME_FOR_HANDLING)

#define PIN_SENSOR_CONTROL        true
#define PIN_TELEMETRY_TRANSMITTER true

struct SensorControlEvent
{
	enum
	{
		// Control Bits 
		StartMeasurement             = 1 << 0,
		StopMeasurement              = 1 << 1,
		// Producing Bits
		PulseOximeterReady           = 1 << 2,
		InertialMeasurementUnitReady = 1 << 3,
		ElectrocardiogramReady       = 1 << 4,
		// Any
		Any                          = StartMeasurement | 
		                               StopMeasurement | 
		                               PulseOximeterReady | 
		                               InertialMeasurementUnitReady | 
		                               ElectrocardiogramReady,
	};
};

namespace config
{
	/**
	 * \brief Sensor Control configuration
	 */
	extern TaskHandle_t       SensorControl;
	extern EventGroupHandle_t SensorControlEventGroup;
	constexpr static uint32_t SENSOR_CONTROL_TASK_STACK_SIZE = 20'000;
	constexpr static uint32_t SENSOR_CONTROL_TASK_PRIORITY   = 1;
#if PIN_SENSOR_CONTROL
	constexpr static uint32_t SENSOR_CONTROL_TASK_CORE       = 1;
#endif
	/**
	 * \brief Telemetry Transmitter configuration
	 */
	extern TaskHandle_t TelemetryTransmitter;
	constexpr static uint32_t TELEMETRY_TRANSMITTER_TASK_STACK_SIZE = 20'000;
	constexpr static uint32_t TELEMETRY_TRANSMITTER_TASK_PRIORITY   = 2;
#if PIN_TELEMETRY_TRANSMITTER
	constexpr static uint32_t TELEMETRY_TRANSMITTER_TASK_CORE       = 1;
#endif
}
