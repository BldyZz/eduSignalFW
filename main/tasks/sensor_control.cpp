#include "esp_util/i2cMaster.hpp"
#include "esp_util/spiHost.hpp"
#include "esp_timer.h"
#include "esp_event.h"

#include "../devices/ADS1299.hpp"
#include "../devices/MAX30102.hpp"
#include "../devices/BHI160.hpp"
#include "../devices/MCP3561.hpp"
#include "../devices/PCF8574.hpp"
#include "../devices/TSC2003.hpp"
#include <cstdio>

#include "task_config.h"
#include "sensor_control.h"
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#define SENSOR_CONTROL_TAG "[Sensor Control:]"

namespace sys
{
	/**
	 *	Globals
	 */
	// Sensor View
	mem::SensorView<mem::int24_t> gSensorView;

	// SPI, I2C Interfaces
	esp::spiHost<config::ADS1299::Config> boardSPI;
	volatile esp::i2cMaster<config::I2C0_Config> boardI2C;
	// Devices I2C
	device::MAX30102 pulseOxiMeter;
	device::BHI160   imu;
	device::PCF8574  ioExpander;
	device::TSC2003  touchScreenController;
	// Devices SPI
	device::ADS1299  ecg(boardSPI);
	device::MCP3561  adc(boardSPI);

	void sensor_control_task(void*)
	{
		PRINTI(SENSOR_CONTROL_TAG, "Initializing...\n");

		// Setup devices
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
	
		while(true)
		{
			ecg.Handler();
			pulseOxiMeter.Handler();
			imu.Handler();
			adc.Handler();
			touchScreenController.Handler();
			YIELD_FOR(4);
		}
	}
}
