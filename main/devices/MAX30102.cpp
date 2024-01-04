#include "MAX30102.hpp"

#include "../util/utils.h"
#include "../util/time.h"

#include <cstdio>
#include <algorithm>
#include <cstring>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

namespace device
{
	struct MAX30102::Register
	{
		enum : util::byte
		{
			InterruptStatus1   = 0x00,
			InterruptStatus2   = 0x01,
			InterruptEnable1   = 0x02,
			InterruptEnable2   = 0x03,
			FiFoWrite          = 0x04,
			OverflowCounter    = 0x05,
			FiFoRead           = 0x06,
			FiFoDataRegister   = 0x07,
			FiFoConfig         = 0x08,
			ModeConfig         = 0x09,
			SPO2Config         = 0x0A,
			LED1PulseAmplitude = 0x0C,
			LED2PulseAmplitude = 0x0D,
			LEDMode1           = 0x11,
			LEDMode2           = 0x12,
			DieTempInteger     = 0x1F,
			DieTempFraction    = 0x20,
			DieTempConfig      = 0x21,
			RevisionID         = 0xFE,
			PartID             = 0xFF,
		};
	};

	enum class MAX30102::State : util::byte
	{
		Reset,
		Init,
		Idle,
		ReadData,
	};

	struct MAX30102::Command
	{
		enum : util::byte
		{
			Reset = 0b0100'0000
		};
	};

	MAX30102::MAX30102()
		:  sample{},
	      _nextTime(timepoint_t::clock::now()),
	      _numberOfSamples(0),
		  _mutexBuffer(),
	      _state(State::Reset)
	{
	}

	void MAX30102::Init()
	{
		PRINTI("[MAX30102:]", "Initialization successful.\n");
		while(!IsReady()) 
			Handler();
	}

	mem::SensorData<mem::int24_t> MAX30102::Data()
	{
		return 	mem::SensorData(&sample.red, config::MAX30102::CHANNEL_COUNT, config::MAX30102::SAMPLE_RATE);
	}

	bool MAX30102::IsReady() const
	{
		return util::to_underlying(_state) >= util::to_underlying(State::Idle);
	}

	void MAX30102::Reset()
	{
		static constexpr util::byte resetPackage[] = {Command::Reset};
		this->write(util::to_span(resetPackage));
	}

	void MAX30102::Configure()
	{
		static constexpr util::byte modeControl = 0b011;
		static constexpr util::byte modeControlPackage[] = {Register::ModeConfig, modeControl};
		this->write(util::to_span(modeControlPackage));

		static constexpr util::byte sampleRate = 0b001;
		static constexpr util::byte pulseWidth = 0b11;
		static constexpr util::byte adcRange = 0b11;
		static constexpr util::byte spo2ConfigData = adcRange << 5 | sampleRate << 2 | pulseWidth;
		static constexpr util::byte spo2Package[] = {Register::SPO2Config, spo2ConfigData};
		this->write(util::to_span(spo2Package));

		static constexpr util::byte sampleAverage = 0b001;
		static constexpr util::byte fifoRolloverEnable = 0b1;
		static constexpr util::byte fifoAFull = 0b0000;
		static constexpr util::byte fifoConfigData = sampleAverage << 5 | fifoRolloverEnable << 4 | fifoAFull;
		static constexpr util::byte fifoPackage[] = {Register::FiFoConfig, fifoConfigData};
		this->write(util::to_span(fifoPackage));

		static constexpr util::byte ledBrightness = 128;
		static constexpr util::byte led1AmplitudePackage[] = {Register::LED1PulseAmplitude, ledBrightness};
		static constexpr util::byte led2AmplitudePackage[] = {Register::LED2PulseAmplitude, ledBrightness};
		this->write(util::to_span(led1AmplitudePackage));
		this->write(util::to_span(led2AmplitudePackage));
	}

	void MAX30102::ReadData()
	{
		std::array<util::byte, 6> rxData;
		this->read(Register::FiFoDataRegister, util::total_size(rxData), rxData.data());
		std::ranges::reverse(rxData);

		sample_t tempRed, tempInfraRed;
		std::memcpy(&tempRed._value, rxData.data() + sizeof(sample_t), sizeof(sample_t));
		std::memcpy(&tempInfraRed._value, rxData.data(), sizeof(sample_t));
		// sample data has a maximum width of 18 Bits, so discard the rest.
		tempRed._value[2]	   &= 0x03;
		tempInfraRed._value[2] &= 0x03; 

		sample = oxi_sample
		{
			.red = tempRed,
			.infraRed = tempInfraRed
		};
		
		_numberOfSamples--;
		_nextTime = timepoint_t::clock::now() + std::chrono::milliseconds(config::sample_rate_to_us_with_deviation(config::MAX30102::SAMPLE_RATE));
	}

	void MAX30102::ReadBufferSize()
	{
		std::uint8_t ReadPointer = 0, WritePointer = 0;
		this->read(Register::FiFoRead, 1, &ReadPointer);
		this->read(Register::FiFoWrite, 1, &WritePointer);
		_numberOfSamples = WritePointer - ReadPointer;
	}

	bool MAX30102::IsEmpty() const
	{
		return _numberOfSamples == 0;
	}

	void MAX30102::Handler()
	{
		switch(_state)
		{
		case State::Reset:
			Reset();
			PRINTI("[MAX30102:]", "Resetting...\n");
			_state = State::Init;
			break;
		case State::Init:
			Configure();
			_state = State::Idle;
			PRINTI("[MAX30102:]", "Configuration successful.\n");
			break;
		case State::Idle:
		{
			ReadBufferSize();
			const timepoint_t now = timepoint_t::clock::now();
			if(IsEmpty())
			{
				break;
			}

			if(_numberOfSamples < 0)
			{
				_numberOfSamples += 32;
			}
			_state = State::ReadData;
		}
			nobreak;
		case State::ReadData:
			ReadData();
			if(_numberOfSamples == 0)
			{
				_state = State::Idle;
			}
			break;
		}
	}
}
