#pragma once

#include "esp_util/spiDevice.hpp"

#include "../config/devices.h"
#include "../util/types.h"

#include <chrono>

namespace device
{
	class MCP3561 : esp::spiDevice<config::MCP3561::Config, config::MCP3561::SPI_MAX_TRANSACTION_LENGTH>
	{
	public:
		explicit MCP3561(esp::spiHost<config::MCP3561::Config> const& bus);

		void Init();
	private:
		using tp = std::chrono::time_point<std::chrono::system_clock>;
		enum class State   : util::byte;
		struct Command;
		struct Register;

		using dc_t = int32_t;

		void Reset();
		void PowerUp();
		void Configure();
		void CaptureData();
		void Handler();

		State _state;
		tp _resetTime;
		std::size_t _errorCounter;
		dc_t _output;
	};
}