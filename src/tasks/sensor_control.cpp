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

namespace sys
{
	void sensor_control_task(WRITE_ONLY void* args)
	{
		fmt::print("[SensorControlTask:] Initializing...\n");

		// Create SPI, I2C Interfaces
		esp::spiHost<config::ADS1299::Config> boardSPI;
		fmt::print("[SensorControlTask:] Initialized SPI.\n");
		esp::i2cMaster<I2C0_Config> boardI2C;
		fmt::print("[SensorControlTask:] Initialized I2C.\n");

		// Create and initialize Sensors
		device::MAX30102 pulseOxiMeter;
		device::ADS1299  ecg(boardSPI);
		device::BHI160   imu;
		device::PCF8574  ioExpander;
		device::MCP3561  adc(boardSPI);
		device::TSC2003  touchScreenController;

		pulseOxiMeter.Init();
		ecg.Init();
		imu.Init();
		ioExpander.Init();
		adc.Init();
		touchScreenController.Init();

		while(!ecg.IsReady()) ecg.Handler();
		while(!imu.IsReady()) imu.Handler();
		while(!pulseOxiMeter.IsReady()) pulseOxiMeter.Handler();

		// ReSharper disable once CppTooWideScope
		mem::ring_buffer_t sensor_buffers[] =
		{
			//adc.RingBuffer(),
			//pulseOxiMeter.RingBuffer(),
			//ads.ECGRingBuffer(),
			imu.RingBuffer(),
		};

		// Pass back ring buffer. 
		{
			mem::ring_buffer_t** ringBufferInfoPtrPtr = static_cast<mem::ring_buffer_t**>(args);
			*ringBufferInfoPtrPtr = sensor_buffers;
			ringBufferInfoPtrPtr++;
			size_t* numberOfRingBuffers = reinterpret_cast<size_t*>(ringBufferInfoPtrPtr);
			*numberOfRingBuffers = std::size(sensor_buffers);
		}

		// Start Measuring
		while(true)
		{
			//do
			//{
			//	ads.Handler();
			//} while(!ads.IsReady());

			//do
			//{
			//	pulseOxiMeter.Handler();
			//} while(!pulseOxiMeter.IsReady());

			do
			{
				imu.Handler();
			} while(!imu.IsReady());
		}
	}
}