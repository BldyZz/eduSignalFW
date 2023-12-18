#include "esp_util/i2cMaster.hpp"
#include "esp_util/spiHost.hpp"

#include "../memory/ring_buffer.h"
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

	void sensor_control_task(WRITE_ONLY void* view)
	{
		std::printf("[SensorControlTask:] Initializing...\n");

		pulseOxiMeter.Init();
		ecg.Init();
		imu.Init();
		ioExpander.Init();
		adc.Init();
		touchScreenController.Init();

		mem::RingBufferView::handle sensorBuffers[] =
		{
			pulseOxiMeter.RingBuffer(),
			ecg.ECGRingBuffer(),
			imu.RingBuffer(),
			//adc.RingBuffer(),
		};
		file::bdf_signal_header_t adsHeaders[config::ADS1299::CHANNEL_COUNT];
		file::bdf_signal_header_t pulseOxiMeterHeaders[config::MAX30102::CHANNEL_COUNT];
		file::bdf_signal_header_t imuHeaders[config::BHI160::CHANNEL_COUNT];
		file::createBDFHeader<config::ADS1299>(adsHeaders);
		file::createBDFHeader<config::MAX30102>(pulseOxiMeterHeaders);
		file::createBDFHeader<config::BHI160>(imuHeaders);
		sensorBuffers[0]->SetBDF(pulseOxiMeterHeaders, config::MAX30102::NODES_IN_BDF_RECORD);
		sensorBuffers[1]->SetBDF(adsHeaders, config::ADS1299::NODES_IN_BDF_RECORD);
		sensorBuffers[2]->SetBDF(imuHeaders, config::BHI160::NODES_IN_BDF_RECORD);

		// Pass back ring buffer. 
		*static_cast<mem::RingBufferView*>(view) = mem::RingBufferView(sensorBuffers, std::size(sensorBuffers));

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
