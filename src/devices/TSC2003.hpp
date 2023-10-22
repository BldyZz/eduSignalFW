#pragma once

// std
#include <chrono>
// internal
#include "../i2cConfig.hpp"
#include "../util/types.h"
#include "../config/devices.h"
// external

#include "esp_util/i2cDevice.hpp"

namespace device
{
	/**
	 * \brief 4-wire resistive touch screen controller with direct battery measurement and temperature sensor
	 */
	class TSC2003 : esp::i2cDevice<config::TSC2003::Config, config::TSC2003::ADDRESS>
	{
	public:
		TSC2003();

		void Init();


		std::int16_t Temperature() const;
	private:
		void Handler();

		// Constants
		static constexpr auto CONVERSION_TIME{std::chrono::microseconds(20)};

		// Types
		enum class State : util::byte;
		struct MeasurementCommand;
		struct ConfigurationCommand;
		using tp = std::chrono::time_point<std::chrono::system_clock>;

		// Fields
		std::int16_t                                    _temperature;
		State                                           _state;
		tp _conversionDone;
	};
}