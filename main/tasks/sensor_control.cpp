#include "esp_util/i2cMaster.hpp"
#include "esp_util/spiHost.hpp"

#include "../devices/ADS1299.hpp"
#include "../devices/MAX30102.hpp"
#include "../devices/BHI160.hpp"
#include "../devices/MCP3561.hpp"
#include "../devices/PCF8574.hpp"
#include "../devices/TSC2003.hpp"
#include "../network/bdf_plus.h"

#include "sensor_control.h"
#include "task_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include <cstdio>

namespace sys
{
	mem::SensorView<mem::int24_t> gSensorView;

	// SPI, I2C Interfaces
	esp::spiHost<config::ADS1299::Config> boardSPI;
	volatile esp::i2cMaster<config::I2C0_Config> boardI2C;
	// Devices
	device::MAX30102 pulseOxiMeter;
	device::ADS1299  ecg(boardSPI);
	device::BHI160   imu;
	device::PCF8574  ioExpander;
	device::MCP3561  adc(boardSPI);
	device::TSC2003  touchScreenController;

	TimerHandle_t create_device_timer()
	{
		return xTimerCreate("Timer", pdMS_TO_TICKS(config::sample_rate_to_ms(config::MAX30102::SAMPLE_RATE)), true, nullptr, [](TimerHandle_t)
		{
			pulseOxiMeter.Handler();
		});
	}

	void foo(WRITE_ONLY void* view)
	{
		pulseOxiMeter.Init();
		ecg.Init();
		imu.Init();
		ioExpander.Init();
		adc.Init();
		touchScreenController.Init();
		constexpr uint32_t NUMBER_OF_TIMERS = 3;
		TimerHandle_t timers[NUMBER_OF_TIMERS];

		

		for(uint32_t currentTimer = 0; currentTimer < NUMBER_OF_TIMERS; ++currentTimer)
		{
			
		}
	}

	void sensor_control_task(void*)
	{
		std::printf("[SensorControlTask:] Initializing...\n");

		pulseOxiMeter.Init();
		ecg.Init();
		imu.Init();
		ioExpander.Init();
		adc.Init();
		touchScreenController.Init();

		std::array sensorBuffers =
		{
			ecg.ECGData(),
			pulseOxiMeter.Data(),
			imu.Data(),
			//adc.Data(),
		};

		gSensorView = mem::SensorView<mem::int24_t>(sensorBuffers.begin(), sensorBuffers.end());

		// Start Measuring
		while(true)
		{
			pulseOxiMeter.Handler();
			ecg.Handler();
			imu.Handler();
			adc.Handler();
			touchScreenController.Handler();
			//xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		}
	}
}
