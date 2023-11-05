#pragma once

// external
#include "esp_util/i2cDevice.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
// std
#include <cstdint>
// internal
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

		bool IsReady() const;
		void Handler();
	private:
		using sample_t = mem::uint24_t;

		enum class State : util::byte;
		struct Register;
		struct Command;

		void Reset();
		void Configure();
		void ReadData();

		sample_t          _underlyingBuffer[2 * config::MAX30102::SAMPLES_IN_RING_BUFFER]; // Don't use directly! Use _buffer instead.
		mem::RingBuffer   _buffer;
		int32_t           _numberOfSamples;
		StaticSemaphore_t _mutexBuffer{};
		State             _state;
	};
}