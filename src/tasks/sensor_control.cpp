#include "esp_util/i2cMaster.hpp"
#include "esp_util/spiHost.hpp"

#include "../memory/ring_buffer.h"
#include "../devices/ADS1299.hpp"
#include "../devices/MAX30102.hpp"
#include "../devices/BHI160.hpp"
#include "../devices/MCP3561.hpp"
#include "../devices/PCF8574.hpp"
#include "../devices/TSC2003.hpp"

#include "sensor_control.h"
#include "task_config.h"

#include <cstdio>

namespace sys
{
	void sensor_control_task(WRITE_ONLY void* view)
	{
		std::printf("[SensorControlTask:] Initializing...\n");

		// Create SPI, I2C Interfaces
		esp::spiHost<config::ADS1299::Config> boardSPI;
		std::printf("[SensorControlTask:] Initialized SPI.\n");
		esp::i2cMaster<config::I2C0_Config> boardI2C;
		std::printf("[SensorControlTask:] Initialized I2C.\n");

		// Create and initialize Sensors
		device::MAX30102 pulseOxiMeter;
		device::ADS1299  ecg(boardSPI);
		device::BHI160   imu;
		device::PCF8574  ioExpander;
		device::MCP3561  adc(boardSPI);
		device::TSC2003  touchScreenController;

		pulseOxiMeter.Init();
		ecg.Init();
		//imu.Init();
		//ioExpander.Init();
		//adc.Init();
		//touchScreenController.Init();
		
		while(!pulseOxiMeter.IsReady()) pulseOxiMeter.Handler();

		// ReSharper disable once CppTooWideScope	
		mem::RingBufferView::handle sensor_buffers[] =
		{
			pulseOxiMeter.RingBuffer(),
			ecg.ECGRingBuffer(),
			//imu.RingBuffer(),
			//adc.RingBuffer(),
		};

		// Pass back ring buffer. 
		*static_cast<mem::RingBufferView*>(view) = mem::RingBufferView(sensor_buffers, std::size(sensor_buffers));

		// Timer Create

		// Start Measuring
		while(true)
		{
			pulseOxiMeter.Handler();
			ecg.Handler();
			//imu.Handler();

			WATCHDOG_HANDLING();
		}
	}
}