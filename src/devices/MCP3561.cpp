#include "MCP3561.hpp"

#include <algorithm>
#include <array>
#include <cstdio>

#include "../util/utils.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

namespace device
{
	enum class MCP3561::State : util::byte
	{
		Reset,
		PowerUp,
		Config,
		Idle,
		CaptureData,
		Shutdown
	};

	struct MCP3561::Register
	{
		enum : util::byte
		{
			ADCDATA = 0x0,
			CONFIG0 = 0x1,
			CONFIG1 = 0x2,
			CONFIG2 = 0x3,
			CONFIG3 = 0x4,
			IRQ = 0x5,
			MUX = 0x6,
			SCAN = 0x7,
			TIMER = 0x8,
			OFFESETCAL = 0x9,
			GAINCAL = 0xA,
			LOCK = 0xD,
			CRCCFG = 0xF,
		};
	};

	struct MCP3561::Command
	{
		static constexpr util::byte	StartConversion[] = {config::MCP3561::ADDRESS << 6 | 0b1010'00};
		static constexpr util::byte	Standby[] = {config::MCP3561::ADDRESS << 6 | 0b1011'00};
		static constexpr util::byte	Shutdown[] = {config::MCP3561::ADDRESS << 6 | 0b1100'00};
		static constexpr util::byte	FullShutdown[] = {config::MCP3561::ADDRESS << 6 | 0b1101'00};
		static constexpr util::byte	FullReset[] = {config::MCP3561::ADDRESS << 6 | 0b1110'00};
		static constexpr util::byte StaticRead(util::byte Address)
		{
			return config::MCP3561::ADDRESS << 6 | (Address << 2) | 0b01;
		};
		static constexpr util::byte IncrementalWrite(util::byte Address)
		{
			return config::MCP3561::ADDRESS << 6 | (Address << 2) | 0b10;
		};
		static constexpr util::byte IncrementalRead(util::byte Address)
		{
			return config::MCP3561::ADDRESS << 6 | (Address << 2) | 0b11;
		};
	};

	MCP3561::MCP3561(esp::spiHost<config::MCP3561::Config> const& bus)
		: esp::spiDevice<config::MCP3561::Config, config::MCP3561::SPI_MAX_TRANSACTION_LENGTH>(bus, config::MCP3561::CLOCK_SPEED, config::MCP3561::CS_PIN, config::MCP3561::SPI_MODE),
		_errorCounter(0)
	{
	}

	void MCP3561::Init()
	{
		_buffer = mem::RingBuffer(&_mutexBuffer, _output, config::MCP3561::ID);
		gpio_set_direction(config::MCP3561::IRQ_PIN, GPIO_MODE_INPUT);
		PRINTI("[MCP3561:]", "Initialization complete.\n");
	}

	mem::RingBuffer* MCP3561::RingBuffer()
	{
		if(!_buffer.IsValid())
		{
			PRINTI("[ADS1299:]", "Ring buffer was not initialized!\n");
		}
		return &_buffer;
	}

	void MCP3561::Reset()
	{
		sendBlocking(util::to_span(Command::FullReset));
		_resetTime = std::chrono::system_clock::now() + std::chrono::microseconds(200);
		PRINTI("[MCP3561:]", "Resetting...\n");
	}

	void MCP3561::PowerUp()
	{
		auto now{std::chrono::system_clock::now()};
		if(now > _resetTime)
		{
			std::array<util::byte, 2> rxData{};
			static constexpr util::byte setConfig0[] = {Command::StaticRead(Register::CONFIG0), 0x00};
			this->sendBlocking(util::to_span(setConfig0), rxData);
			if(rxData[1] == 0xC0)
			{
				PRINTI("[MCP3561:]", "Power up complete!\n");
				_errorCounter = 0;
				_state = State::Config;
			} else
			{
				PRINTI("[MCP3561:]", "Power up failed! Chip not responding! Retrying...\n");
				++_errorCounter;
				_state = State::Reset;
			}
		}
		if(_errorCounter > 10)
		{
			PRINTI("[MCP3561:]", "Too many errors... Shutting down!\n");
			PRINTI("[MCP3561:]", "Check all voltages on Chip! Maybe analog or digital supply missing!\n");
			_state = State::Shutdown;
		}
	}

	void MCP3561::Configure()
	{
		static constexpr util::byte configuration[] =
		{
			Command::IncrementalWrite(Register::CONFIG0),
			//CONFIG0 
			0xe3,
			//CONFIG1
			0x14,
			//CONFIG2 
			0x8f,
			//CONFIG3
			0xe0,
			//IRQ
			0x77,
			//MUX
			0x88,
			//SCAN
			0x00,
			0x00,
			0x00,
			//TIMER
			0x00,
			0x00,
			0x00,
			//OFFSETCAL
			0x00,
			0x03,
			0x7A,
			//GAINCAL
			0x00,
			0x00,
			0x00,
		};
		this->sendBlocking(util::to_span(configuration));
	}

	void MCP3561::CaptureData()
	{
		static constexpr auto readSize{1 + 4};
		static constexpr util::byte txData[readSize] = {Command::IncrementalRead(Register::ADCDATA), util::PADDING_BYTE, util::PADDING_BYTE, util::PADDING_BYTE, util::PADDING_BYTE};

		std::array<util::byte, readSize> rxData;
		rxData.fill({});

		this->sendBlocking(util::to_span(txData), rxData);
		std::int32_t transformedData = 0;
		std::reverse_copy(rxData.begin(), rxData.end(), &transformedData);


		//transformedData = transformedData << 8;
		//transformedData	 = transformedData >> 8;
		//fmt::print("{} {:#010b}\n", std::chrono::steady_clock::now().time_since_epoch() ,fmt::join(rxData, ", "));
		_buffer.Lock();
		*static_cast<mem::int24_t*>(_buffer.WriteAdvance()) = mem::int24_t(transformedData);
		_buffer.Unlock();
	}

	void MCP3561::Handler()
	{
		switch(_state)
		{
		case State::Reset:
			Reset();
			_state = State::PowerUp;
			break;
		case State::PowerUp:
			PowerUp();
			break;
		case State::Config:
			Configure();
			_state = State::Idle;
			break;
		case State::Idle:
			if(gpio_get_level(config::MCP3561::IRQ_PIN) == 0)
			{
				_state = State::CaptureData;
			}
			break;
		case State::CaptureData:
			CaptureData();
			_state = State::Idle;
			break;
		case State::Shutdown:
			break;
		default:;
		}
	}
}
