#include "MAX30102.hpp"

#include "../util/utils.h"

namespace device
{
	struct MAX30102::Register
	{
		enum : util::byte
		{
			InterruptStatus1 = 0x00,
			InterruptStatus2 = 0x01,
			InterruptEnable1 = 0x02,
			InterruptEnable2 = 0x03,
			FiFoWrite = 0x04,
			OverflowCounter = 0x05,
			FiFoRead = 0x06,
			FiFoDataRegister = 0x07,
			FiFoConfig = 0x08,
			ModeConfig = 0x09,
			SPO2Config = 0x0A,
			LED1PulseAmplitude = 0x0C,
			LED2PulseAmplitude = 0x0D,
			LEDMode1 = 0x11,
			LEDMode2 = 0x12,
			DieTempInteger = 0x1F,
			DieTempFraction = 0x20,
			DieTempConfig = 0x21,
			RevisionID = 0xFE,
			PartID = 0xFF,
		};
	};

	enum class MAX30102::State : util::byte
	{
		Reset,
		Init,
		Idle,
		ReadData,
		ChangeConfig
	};

	struct MAX30102::Command
	{
		enum : util::byte
		{
			Reset = 0b0100'0000
		};
	};

	MAX30102::MAX30102()
		: _state(State::Reset), _numberOfSamples(0), _underlyingBuffer{}
	{
	}

	void MAX30102::Init()
	{
		_buffer = mem::createStaticRingBuffer(_underlyingBuffer, &_mutexBuffer);
		fmt::print("[MAX30102:] Initialization successful.\n");

	}

	mem::ring_buffer_t MAX30102::RingBuffer() const
	{
		if(!_buffer.buffer)
		{
			fmt::print("[MAX30102:] Ring buffer was not initialized!\n");
		}
		return _buffer;
	}

	bool MAX30102::IsReady() const
	{
		return _state == State::Idle;
	}

	void MAX30102::Reset()
	{
		static constexpr util::byte resetPackage[] = {Register::ModeConfig, Command::Reset};
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
		static constexpr util::byte fifoRolloverEnable = 0b0;
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
		util::byte rxData[6];
		this->read(Register::FiFoDataRegister, util::total_size(rxData), rxData);
		std::reverse(std::begin(rxData), std::end(rxData));

		std::uint32_t tempRed = 0;
		std::memcpy(&tempRed, &rxData[3], 3);
		tempRed &= 0x3FFFF;

		std::uint32_t tempInfraRed = 0;
		std::memcpy(&tempInfraRed, &rxData[0], 3);
		tempInfraRed &= 0x3FFFF;

		mem::write(&_buffer, sample_t{.red = tempRed, .infraRed = tempInfraRed});

		_numberOfSamples--;
	}

	void MAX30102::PollFIFO()
	{
		//TODO: Get Read and Write Pointer and calculate available Samples
		std::uint8_t ReadPointer;
		std::uint8_t WritePointer;
		this->read(Register::FiFoRead, 1, &ReadPointer);
		this->read(Register::FiFoWrite, 1, &WritePointer);
		if(ReadPointer != WritePointer)
		{
			_numberOfSamples = WritePointer - ReadPointer;
			if(_numberOfSamples < 0)
			{
				_numberOfSamples += 32;
			}
			_state = State::ReadData;
		}
	}

	void MAX30102::Handler()
	{
		switch(_state)
		{
		case State::Reset:
			Reset();
			fmt::print("[MAX30102:] Resetting...\n");
			_state = State::Init;
			break;
		case State::Init:
			Configure();
			_state = State::Idle;
			fmt::print("[MAX30102:] Configuration successful.\n");
			break;
		case State::Idle:
			PollFIFO();
			break;
		case State::ReadData:
			ReadData();
			_state = State::ChangeConfig;
			break;
		case State::ChangeConfig:
			//TODO: Update configuration of the sensor while measuring...
			_state = _numberOfSamples > 0 ? State::ReadData : State::Idle;
			break;
		}
	}
}
