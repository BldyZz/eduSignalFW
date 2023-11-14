// external
// std
#include <cstdio>
// internal
#include "../util/utils.h"

#include "TSC2003.hpp"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

namespace device
{
	enum class TSC2003::State : util::byte
	{
		Reset,
		Idle,
		SetConversion,
		//WaitForResult,
		GetResult
	};

	struct TSC2003::MeasurementCommand
	{
		static constexpr util::byte TEMP0[] = {0b0000'1000};
		static constexpr util::byte VBAT1[] = {0b0001'1000};
		static constexpr util::byte IN1[] = {0b0010'1000};
		static constexpr util::byte TEMP1[] = {0b0100'1000};
		static constexpr util::byte VBAT2[] = {0b0101'1000};
		static constexpr util::byte IN2[] = {0b0110'1000};
		static constexpr util::byte XPosition[] = {0b1100'1000};
		static constexpr util::byte YPosition[] = {0b1101'1000};
		static constexpr util::byte Z1Position[] = {0b1110'1000};
		static constexpr util::byte Z2Position[] = {0b1111'1000};
	};

	struct TSC2003::ConfigurationCommand
	{
		static constexpr util::byte XNegDrivers[] = {0b1000'1000};
		static constexpr util::byte YNegDrivers[] = {0b1001'1000};
		static constexpr util::byte YPosXNegDrivers[] = {0b1010'1000};
	};

	TSC2003::TSC2003()
		:
		_temperature(std::numeric_limits<decltype(_temperature)>::max()),
		_state(State::Idle)
	{
	}

	void TSC2003::Init()
	{
		gpio_set_direction(config::TSC2003::NIRQ_PIN, GPIO_MODE_INPUT);
		gpio_set_pull_mode(config::TSC2003::NIRQ_PIN, GPIO_PULLUP_ONLY);

		_state = State::SetConversion;
		PRINTI("[TSC2003:]", "Initialization successful.\n");
	}

	void TSC2003::Handler()
	{
		switch(_state)
		{
		case State::Reset:
			_state = State::SetConversion;
			break;
		case State::SetConversion:
			this->write(MeasurementCommand::VBAT1);
			_state = State::GetResult;
			break;
		case State::GetResult:
			if(std::chrono::system_clock::now() > _conversionDone)
			{
				util::byte temperature[2];
				this->readOnly(std::size(temperature), temperature);
				_temperature = static_cast<decltype(_temperature)>(temperature[0] << 8 | temperature[1]); // Reverse and store the temperature
				_state = State::SetConversion;
			}
			break;
		case State::Idle:
			break;
		}
	}
}

