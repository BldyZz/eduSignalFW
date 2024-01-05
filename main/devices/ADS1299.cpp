// extern
#include "freertos/FreeRTOS.h" // std
// intern
#include "../util/utils.h"
#include "../util/defines.h"
#include "../util/time.h"

#include <array>
#include <algorithm>
#include <cstdio>

#include "ADS1299.hpp"

namespace device
{

	enum class ADS1299::State : util::byte
	{
		Reset,
		PowerUp,
		WaitForPower,
		SetExternalReference,
		SetNoiseData,
		CaptureNoiseData,
		SetTestSignals,
		CaptureTestData,
		SetCustomSettings,
		Idle,
		CaptureData,
		FatalError,
		Shutdown,
	};

	struct ADS1299::Register
	{
		enum : util::byte
		{
			Id = 0x00,
			Config1 = 0x01,
			Config2 = 0x02,
			Config3 = 0x03,
			Loff = 0x04,
			Ch1Set = 0x05,
			Ch2Set = 0x06,
			Ch3Set = 0x07,
			Ch4Set = 0x08,
			Ch5Set = 0x09,
			Ch6Set = 0x0A,
			Ch7Set = 0x0B,
			Ch8Set = 0x0C,
			BiasSensp = 0x0D,
			BiasSensn = 0x0E,
			LoffSensp = 0x0F,
			LoffSensn = 0x10,
			LoffFlip = 0x11,
			LoffStatp = 0x12,
			LoffStatn = 0x13,
			Gpio = 0x14,
			Misc1 = 0x15,
			Misc2 = 0x16,
			Config4 = 0x17,
		};
	};

	struct ADS1299::Command
	{
		static constexpr util::byte WakeUp[] = {0x02};
		static constexpr util::byte Standby[] = {0x04};
		static constexpr util::byte Reset[] = {0x06};
		static constexpr util::byte Start[] = {0x08};
		static constexpr util::byte Stop[] = {0x0A};
		static constexpr util::byte RDataC[] = {0x10};
		static constexpr util::byte SDataC[] = {0x11};
		static constexpr util::byte RData[] = {0x12};
		static constexpr util::byte NOP[] = {0xFF};
	};

	struct ADS1299::Config1Flags
	{
		enum : util::byte
		{
			RESERVED = 0x90,	   // Must be set.
			NOT_DAISY_EN = 1 << 6, // Multiple readback mode (Daisy-chain mode otherwise)
			CLK_EN = 1 << 5,	   // Oscillator clock enabled
			DR_000 = 0b000,		   // Output Data Rate: 16 kSPS
			DR_001 = 0b001,		   // Output Data Rate: 8 kSPS
			DR_010 = 0b010,		   // Output Data Rate: 4 kSPS
			DR_011 = 0b011,		   // Output Data Rate: 2 kSPS
			DR_100 = 0b100,		   // Output Data Rate: 1 kSPS
			DR_101 = 0b101,		   // Output Data Rate: 500 SPS
			DR_110 = 0b110,		   // Output Data Rate: 250 SPS
								   /* DR_111 is reserved */
		};
	};

	struct ADS1299::Config2Flags
	{
		enum : util::byte
		{
			RESERVED = 0xC0,  // Must be set.
			INT_CAL = 1 << 4, // Test signals generated internally.
			CAL_AMP = 1 << 2,
			CAL_FREQ_00 = 0b00, // f_CLK / 2^21
			CAL_FREQ_01 = 0b01, // f_CLK / 2^20
			CAL_FREQ_11 = 0b11, // At DC
		};
	};

	struct ADS1299::Config3Flags
	{
		enum : util::byte
		{
			NOT_PD_REFBUF = 1 << 7,
			RESERVED = 0x60, // Must be set.
			BIAS_MEAS = 1 << 4,
			BIASREF_INT = 1 << 3,
			NOT_PD_BIAS = 1 << 2,
			BIAS_LOFF_SENS = 1 << 1,
			BIAS_STAT = 1 << 0,
		};
	};

	struct ADS1299::ChannelFlags
	{
		enum : util::byte
		{
			PDn = 1 << 7,
			GAINn_000 = 0b000, // Gain setting 1
			SRB2 = 1 << 3,	   // Close SRB2 connection
			MUXn_001 = 0b001,  // Channel Input Selection: 001 : Input shorted (for offset or noise measurements)
			MUXn_101 = 0b101,
		};
	};

	static constexpr util::byte BytesToWrite(util::byte number)
	{
		return (number & 0x1F) - 1;
	}

	static constexpr util::byte GetBitmapOfChannelNumber()
	{
		if constexpr (config::ADS1299::CHANNEL_COUNT == 4)
		{
			return 0b00;
		}
		if constexpr (config::ADS1299::CHANNEL_COUNT == 6)
		{
			return 0b01;
		}
		if constexpr (config::ADS1299::CHANNEL_COUNT == 8)
		{
			return 0b10;
		}
		return 0b11;
	}

	ADS1299::ADS1299(esp::spiHost<config::ADS1299::Config> const &bus)
		: esp::spiDevice<config::ADS1299::Config, config::ADS1299::SPI_MAX_TRANSACTION_LENGTH>(
			  bus, config::ADS1299::CLOCK_SPEED, config::ADS1299::CS_PIN, config::ADS1299::SPI_MODE),
		  _state(State::Reset),
		  _noise{},
		  _ecg{},
		  _nextTime(timepoint_t::clock::now()),
		  _statusBits(0),
		  _resetCounter(0),
		  _mutexBuffer{}
	{
	}

	void ADS1299::Init()
	{
		_state       = State::Reset;

		gpio_set_direction(config::ADS1299::RESET_PIN, GPIO_MODE_OUTPUT);
		gpio_set_direction(config::ADS1299::N_PDWN_PIN, GPIO_MODE_OUTPUT);
		gpio_set_direction(config::ADS1299::N_DRDY_PIN, GPIO_MODE_INPUT);

		while(!IsReady())
			Handler();

		PRINTI("[ADS1299:]", "Initialization successful.\n");
	}

	void ADS1299::CaptureData()
	{
		constexpr size_t TRANSACTION_SIZE = config::ADS1299::CHANNEL_COUNT * sizeof(mem::int24_t);
		std::array<util::byte, TRANSACTION_SIZE> rxData{};
		std::array<util::byte, TRANSACTION_SIZE> txData{0x00};

		sendBlocking(txData, rxData);
		// Reverse data
		auto first = rxData.begin();
		for(util::byte channel = 0; channel < config::ADS1299::CHANNEL_COUNT; ++channel, first = std::next(first, sizeof(voltage_t)))
			std::reverse(first, std::next(first, sizeof(voltage_t)));
		
		std::memcpy(_ecg.channels, rxData.data(), sizeof(ecg_t));
		_nextTime = timepoint_t::clock::now() + std::chrono::milliseconds(config::sample_rate_to_us_with_deviation(config::ADS1299::SAMPLE_RATE));
	}

	void ADS1299::Handler()
	{
		switch (_state)
		{
		case State::Reset:
			Reset();
			_state = State::PowerUp;
			break;
		case State::PowerUp:
			PowerUp();
			_state = State::WaitForPower;
			break;
		case State::WaitForPower:
			WaitForPower();
			_state = State::SetExternalReference;
			break;
		case State::SetExternalReference:
			if (IsPoweredUp())
			{
				ConfigureExternalReference();
				PRINTI("[ADS1299:]", "Communication established!\n");
				_resetCounter = 0;
				_state = State::SetTestSignals; // Wait for internal Reference to settle
			}
			else
			{
				PRINTI("[ADS1299:]", "Could not set up device! Restarting...\n");
				_resetCounter++;
				_state = _resetCounter > 10 ? State::FatalError : State::Reset;
			}
			break;
		case State::SetTestSignals:
			SetTestSignals();
			_state = State::Idle;
			break;
		case State::SetNoiseData:
			SetNoiseData();
			_state = State::Idle;
			break;
		case State::CaptureNoiseData:
			if (gpio_get_level(config::ADS1299::N_DRDY_PIN) == 0)
			{
				// TODO: sample more noise data and check noise
				CaptureData();
				_state = State::SetTestSignals;
			}
			break;
		case State::CaptureTestData:
			if (gpio_get_level(config::ADS1299::N_DRDY_PIN) == 0)
			{
				// TODO: Capture Data and Test Signals
				CaptureData();
				_state = State::Idle;
				//_state = State::SetCustomSettings;
			}
			break;
		case State::SetCustomSettings:
			SetCustomSettings();
			PRINTI("[ADS1299:]", "Config complete!\n");
			_state = State::Idle;
			break;
		case State::Idle:
			{
				const timepoint_t now = timepoint_t::clock::now();
				if (gpio_get_level(config::ADS1299::N_DRDY_PIN) == 1)
				{
					break;
				}
				_state = State::CaptureData;
			}
			nobreak;
		case State::CaptureData:
			CaptureData();
			_state = State::Idle;
			break;
		case State::FatalError:
			PRINTI("[ADS1299:]", "Too many errors... Shutting down!\n");
			PRINTI("[ADS1299:]", "Check all voltages on Chip! Maybe analog or digital supply missing!\n");
			_state = State::Shutdown;
			break;
		case State::Shutdown:
			break;
		default:;
		}

		// xSemaphoreGive(_semaphore);
	}

	bool ADS1299::IsReady() const
	{
		return _state == State::Idle;
	}

	mem::SensorData<mem::int24_t> ADS1299::ECGData()
	{
		return mem::SensorData(_ecg.channels, std::size(_ecg.channels), config::ADS1299::SAMPLE_RATE);
	}

	void ADS1299::Reset()
	{
		gpio_set_level(config::ADS1299::N_PDWN_PIN, 0);
		gpio_set_level(config::ADS1299::RESET_PIN, 0);
		PRINTI("[ADS1299:]", "Resetting...\n");
		vTaskDelay(pdMS_TO_TICKS(10)); // Evil delay, but only necessary on power up, so it doesn't matter.
	}

	void ADS1299::PowerUp()
	{
		gpio_set_level(config::ADS1299::N_PDWN_PIN, 1);
		gpio_set_level(config::ADS1299::RESET_PIN, 1);
		PRINTI("[ADS1299:]", "Powering up...\n");
		vTaskDelay(pdMS_TO_TICKS(10)); // Evil delay, but only necessary on power up, so it doesn't matter.
	}

	void ADS1299::WaitForPower()
	{
		gpio_set_level(config::ADS1299::RESET_PIN, 0);
		vTaskDelay(pdMS_TO_TICKS(10)); // Evil delay, but only necessary on power up, so it doesn't matter.
		gpio_set_level(config::ADS1299::RESET_PIN, 1);
		vTaskDelay(pdMS_TO_TICKS(10)); // Evil delay, but only necessary on power up, so it doesn't matter.
	}

	void ADS1299::ConfigureExternalReference()
	{
		static constexpr util::byte config3RegisterContent = Config3Flags::NOT_PD_REFBUF | Config3Flags::RESERVED;
		static constexpr util::byte sendConfig3[] = {WREG(Register::Config3), BytesToWrite(1), config3RegisterContent};

		this->sendBlocking(util::to_span(sendConfig3));
	}

	bool ADS1299::IsPoweredUp()
	{
		this->sendBlocking(util::to_span(Command::SDataC));

		std::array<util::byte, 3> rxData;
		static constexpr util::byte txRequestPackage[] = {RREG(Register::Id), BytesToWrite(1), util::PADDING_BYTE};
		this->sendBlocking(util::to_span(txRequestPackage), rxData);

		const auto IDRegisterData = rxData[2];
		static constexpr util::byte DeviceIDChannelMask{0x0F};
		static constexpr util::byte DeviceID{0b11};
		static constexpr util::byte DeviceIDCompare{(DeviceID << 2) | GetBitmapOfChannelNumber()};

		return (IDRegisterData & DeviceIDChannelMask) == DeviceIDCompare;
	}

	void ADS1299::SetTestSignals()
	{
		this->sendBlocking({Command::SDataC});

		// Configure Config 2
		static constexpr util::byte config2RegisterContent = Config2Flags::RESERVED | Config2Flags::INT_CAL | Config2Flags::CAL_FREQ_00;
		static constexpr util::byte config2Package[] = {WREG(Register::Config2), BytesToWrite(1), config2RegisterContent};
		this->sendBlocking(util::to_span(config2Package));

		// Configure Channel 1 - 4
		static constexpr util::byte channelSettings = ChannelFlags::SRB2 | ChannelFlags::MUXn_101;
		static constexpr util::byte channelSettingPackage[] = {WREG(Register::Ch1Set), BytesToWrite(4), channelSettings, channelSettings, channelSettings, channelSettings};
		this->sendBlocking(util::to_span(channelSettingPackage));

		this->sendBlocking({Command::Start});
		this->sendBlocking({Command::RDataC});
	}

	void ADS1299::SetNoiseData()
	{
		static constexpr util::byte dataRate = []()
		{
			return Config1Flags::DR_110;
			/*if constexpr(config::ADS1299::SAMPLE_RATE == 250)
				return Config1Flags::DR_110;
			else if constexpr(config::ADS1299::SAMPLE_RATE == 500)
				return Config1Flags::DR_101;
			else if constexpr(config::ADS1299::SAMPLE_RATE == 1'000)
				return Config1Flags::DR_100;
			else if constexpr(config::ADS1299::SAMPLE_RATE == 2'000)
				return Config1Flags::DR_011;
			else if constexpr(config::ADS1299::SAMPLE_RATE == 4'000)
				return Config1Flags::DR_010;
			else if constexpr(config::ADS1299::SAMPLE_RATE == 8'000)
				return Config1Flags::DR_001;
			else if constexpr(config::ADS1299::SAMPLE_RATE == 16'000)
				return Config1Flags::DR_000;*/
		}();
		
		// Configure Config 1
		static constexpr util::byte config1RegisterContent = Config1Flags::RESERVED | dataRate;
		static constexpr util::byte config1Package[] = {WREG(Register::Config1), BytesToWrite(1), config1RegisterContent};
		this->sendBlocking(util::to_span(config1Package));

		// Configure Config 2 to default value
		static constexpr util::byte config2RegisterContent = Config2Flags::RESERVED;
		static constexpr util::byte config2Package[] = {WREG(Register::Config2), BytesToWrite(1), config2RegisterContent};
		this->sendBlocking(util::to_span(config2Package));

		// Set Channel 1 - 4 to input Short
		static constexpr util::byte channelSetting = ChannelFlags::MUXn_001;
		static constexpr util::byte channelSettingPackage[] = {WREG(Register::Ch1Set), BytesToWrite(4), channelSetting, channelSetting, channelSetting, channelSetting};
		this->sendBlocking(util::to_span(channelSettingPackage));
		this->sendBlocking(util::to_span(Command::Start));
		this->sendBlocking(util::to_span(Command::RDataC));
	}

	void ADS1299::SetCustomSettings()
	{
		this->sendBlocking(util::to_span(Command::SDataC));
		//-- Setting up MUX Configuration
		this->sendBlocking(util::to_span(Command::SDataC));
		// ConfigureExternalReference Config2 Register to default value!
		static constexpr util::byte config2RegisterContent = Config2Flags::RESERVED;
		static constexpr util::byte config2Package[] = {WREG(Register::Config2), BytesToWrite(1), config2RegisterContent};
		this->sendBlocking(util::to_span(config2Package));
		// Set all ChannelMux to Gain 1, SRB2 open,
		// Normal electrode input in normal operation mode!
		static constexpr util::byte channelPackage[] = {WREG(Register::Ch1Set), BytesToWrite(4), 0, ChannelFlags::MUXn_001, ChannelFlags::MUXn_001, ChannelFlags::MUXn_001};
		this->sendBlocking(util::to_span(channelPackage));

		this->sendBlocking(util::to_span(Command::Start));
		this->sendBlocking(util::to_span(Command::RDataC));
	}
}
