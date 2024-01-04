#pragma once

// external
#include "esp_util/i2cDevice.hpp"
#include "freertos/FreeRTOS.h"
#include "esp_attr.h"
// std
#include <cstdint>
// internal
#include <chrono>

#include "../config/devices.h"
#include "../util/types.h"
#include "../memory/sensor_data.h"
#include "../memory/int.h"

namespace device
{
	/**
	 * \brief Heart Rate Pulse Blood Oxygen Concentration Sensor
	 */
	class MAX30102 : esp::i2cDevice<config::MAX30102::Config, config::MAX30102::ADDRESS>
	{
	public:
		MAX30102();

		void Init();
		mem::SensorData<mem::int24_t> Data();

		bool IsReady() const;
		void IRAM_ATTR Handler();
	private:
		using sample_t = mem::int24_t;
		using timepoint_t = std::chrono::time_point<std::chrono::system_clock>;
		struct oxi_sample
		{
			sample_t red;
			sample_t infraRed;
		};

		enum class State : util::byte;
		struct Register;
		struct Command;

		void Reset();
		void Configure();
		void ReadData();
		void ReadBufferSize();
		bool IsEmpty() const;

		oxi_sample        sample; 
		timepoint_t       _nextTime;
		int32_t           _numberOfSamples;
		StaticSemaphore_t _mutexBuffer{};
		State             _state;
	};
}