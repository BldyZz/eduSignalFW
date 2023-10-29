#pragma once

// std
#include <chrono>
// internal
#include "esp_util/spiDevice.hpp"
#include "../config/devices.h"
#include "../util/types.h"
#include "../memory/ring_buffer.h"
// external
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace device
{
	class MCP3561 : esp::spiDevice<config::MCP3561::Config, config::MCP3561::SPI_MAX_TRANSACTION_LENGTH>
	{
	public:
		explicit MCP3561(esp::spiHost<config::MCP3561::Config> const& bus);

		void Init();
		void Handler();
		mem::ring_buffer_t* RingBuffer();
	private:
		using tp = std::chrono::time_point<std::chrono::system_clock>;
		enum class State : util::byte;
		struct Command;
		struct Register;

		using dc_t = int32_t;

		void Reset();
		void PowerUp();
		void Configure();
		void CaptureData();

		State _state;
		tp _resetTime;
		std::size_t _errorCounter;
		mem::ring_buffer_t _buffer;
		StaticSemaphore_t  _mutexBuffer;
		dc_t _output[32];
	};
}