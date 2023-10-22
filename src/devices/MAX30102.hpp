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
		mem::ring_buffer_t RingBuffer() const;

		bool IsReady() const;
		void Handler();
	private:
		struct sample_t
		{
			uint32_t red;
			uint32_t infraRed;
		};

		enum class State : util::byte;
		struct Register;
		struct Command;

		void Reset();
		void Configure();
		void ReadData();
		void PollFIFO();

		sample_t _underlyingBuffer[config::MAX30102::SAMPLES_IN_RING_BUFFER]; // Don't use directly! Use _bufferInstead
		mem::ring_buffer_t _buffer;
		int32_t   _numberOfSamples;
		StaticSemaphore_t _mutexBuffer{};
		State _state;
	};
}