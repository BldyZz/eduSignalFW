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
#include "../memory/ring_buffer.h"
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
		mem::RingBuffer* RingBuffer();

		void ReadBufferSize();
		bool HasData();
		bool IsReady() const;
		void ReadData();
		void InsertPadding();

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

		oxi_sample                _underlyingBuffer[config::MAX30102::SAMPLES_IN_RING_BUFFER]; // Don't use directly! Use _buffer instead.
		mem::RingBuffer           _buffer;
		timepoint_t               _nextTime;
		int32_t                   _numberOfSamples;
		StaticSemaphore_t         _mutexBuffer{};
		State                     _state;
	};
}