#pragma once

// external
#include "esp_util/i2cDevice.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
// std
#include <chrono>
// internal
#include "../config/devices.h"
#include "../util/types.h"

namespace device
{
	class PCF8574 : esp::i2cDevice<config::PCFB574::Config, config::PCFB574::ADDRESS>
	{
	public:
		PCF8574();

		void Init();
		void PollTransferData();

		union out_t
		{
			struct
			{
				util::byte bit0 : 1,
					bit1 : 1,
					bit2 : 1,
					bit3 : 1,
					bit4 : 1,
					bit5 : 1,
					bit6 : 1,
					bit7 : 1;
			};
			util::byte value;
		};

		util::byte Input() const;
		out_t Output;
	private:
		using time_point = std::chrono::time_point<std::chrono::system_clock>;

		void TransferData();

		static constexpr auto REFRESH_TIME = std::chrono::milliseconds(200);
		
		util::byte _currentInput;
		util::byte _oldOutput;
		time_point _refreshTimePoint;
	};
}
