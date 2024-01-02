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
#include "../util/utils.h"

#include "sensor_control.h"
#include "../config/task.h"
#include <freertos/timers.h>
#include "esp_timer.h"
#include <cstdio>

#define SENSOR_CONTROL_TAG "[Sensor Control:]"

namespace config
{
	EventGroupHandle_t SensorControlEventGroup = nullptr;
}

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

	consteval TickType_t sample_rate_to_ticks(TickType_t const& sampleRate)
	{
		constexpr TickType_t OFFSET = 2;
		// configTICK_RATE_HZ is 100 Hz. So its rounded up to a multiple of 10ms
		const TickType_t multiples = configTICK_RATE_HZ / sampleRate;
		if(configTICK_RATE_HZ % sampleRate != 0)
		{
			return multiples + 1 + OFFSET;
		}
		return multiples + OFFSET;
	}

	void oxi_meter_timer_callback(TimerHandle_t)
	{
		xEventGroupSetBits(config::SensorControlEventGroup, SensorControlEvent::PulseOximeterReady);
	}

	TimerHandle_t create_oxi_meter_timer()
	{
		pulseOxiMeter.Init();
		return xTimerCreate("Oxi timer", 
							sample_rate_to_ticks(config::MAX30102::SAMPLE_RATE),
							true, 
							nullptr, 
							oxi_meter_timer_callback);
	}

	uint32_t ecg_count = 0;
	void ecg_timer_callback(TimerHandle_t)
	{
		xEventGroupSetBits(config::SensorControlEventGroup, SensorControlEvent::ElectrocardiogramReady);
	}

	TimerHandle_t create_ecg_timer()
	{
		ecg.Init();
		return xTimerCreate("ECG timer", 
							sample_rate_to_ticks(config::ADS1299::SAMPLE_RATE), 
							true, 
							nullptr, 
							ecg_timer_callback);
	}

	uint32_t imu_count = 0;
	void imu_timer_callback(TimerHandle_t)
	{
		xEventGroupSetBits(config::SensorControlEventGroup, SensorControlEvent::InertialMeasurementUnitReady);
	}

	TimerHandle_t create_imu_timer()
	{
		imu.Init();
		return xTimerCreate("IMU timer", 
							sample_rate_to_ticks(config::BHI160::SAMPLE_RATE), 
							true, 
							nullptr, 
							imu_timer_callback);
	}

	uint32_t adc_count = 0;
	void adc_timer_callback(TimerHandle_t h)
	{
		if(adc.HasData())
		{
			adc.CaptureData();
		}
		else
		{
			adc.InsertPadding();
		}
	}

	TimerHandle_t create_adc_timer()
	{
		adc.Init();
		return xTimerCreate("ADC timer", 
							sample_rate_to_ticks(config::MCP3561::SAMPLE_RATE), 
							true, 
							nullptr, 
							adc_timer_callback);
	}

	void sensor_control_task(WRITE_ONLY void* outView)
	{
		ioExpander.Init();
		touchScreenController.Init();

		// Create Event Group
		StaticEventGroup_t eventGroup;
		config::SensorControlEventGroup = xEventGroupCreateStatic(&eventGroup);
		configASSERT(config::SensorControlEventGroup);

		// Create Timers
		const std::array timers = 
		{
			create_oxi_meter_timer(),
			create_ecg_timer(),
			create_imu_timer(),
		};
		//timers[3] = create_adc_timer();

		// Create Sensor View
		mem::RingBufferView::handle sensorBuffers[] =
		{
			pulseOxiMeter.RingBuffer(),
			ecg.ECGRingBuffer(),
			imu.RingBuffer(),
			//adc.RingBuffer(),
		};
		mem::RingBufferView ringBufferView = mem::RingBufferView(sensorBuffers, std::size(sensorBuffers));

		// Create BDF Headers
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
		*static_cast<mem::RingBufferView*>(outView) = ringBufferView;

		auto startTimer = [&]
		{
			ringBufferView.ResetAll();
			std::ranges::for_each(timers, [](TimerHandle_t const& timer)
			{
				if(!xTimerStart(timer, NO_DELAY))
				{
					PRINTI(SENSOR_CONTROL_TAG, "Unable to start timer '%s'.", pcTimerGetName(timer));
				}
			});
		};
		auto stopTimer = [&]
		{
			std::ranges::for_each(timers, [](TimerHandle_t const& timer)
			{
				if(!xTimerStop(timer, NO_DELAY))
				{
					PRINTI(SENSOR_CONTROL_TAG, "Unable to stop timer '%s'.", pcTimerGetName(timer));
				}
			});
		};

		/**
		 * \brief Sensor control loop
		 */
		bool isMeasuring = true;
		while (true)
		{
			// Check for events
			EventBits_t bits = xEventGroupWaitBits(config::SensorControlEventGroup, SensorControlEvent::Any, true, false, portMAX_DELAY);

			if(bits & SensorControlEvent::StartMeasurement)
			{
				startTimer();
			}
			if(bits & SensorControlEvent::StopMeasurement)
			{
				stopTimer();
			}
			if(bits & SensorControlEvent::PulseOximeterReady)
			{
				pulseOxiMeter.HasData() ? pulseOxiMeter.ReadData() : pulseOxiMeter.InsertPadding();
			}
			/*
			if(bits & SensorControlEvent::ElectrocardiogramReady)
			{
				ecg.HasData() ? ecg.CaptureData() : ecg.InsertPadding();
			}
			if(bits & SensorControlEvent::InertialMeasurementUnitReady)
			{
				imu.HasData() ? imu.GetData() : imu.InsertPadding();
			}
			*/
			// Do touch screen and display controlling
			//touchScreenController.Handler();
			//YIELD_FOR(150); 
		}
	}
}
