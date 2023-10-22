#pragma once

// internal
#include "../config/devices.h"
#include "../util/types.h"
#include "../memory/ring_buffer.h"
// external
#include "esp_util/spiDevice.hpp"
#include "esp_util/spiHost.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace device
{
	static size_t ADS1299_MAX_LENGTH_OF_TRANSACTION = 20; // Maximum length of one spi transaction in bytes.

	class ADS1299 : protected esp::spiDevice<config::ADS1299::Config, config::ADS1299::SPI_MAX_TRANSACTION_LENGTH>
	{
	public:
		explicit ADS1299(esp::spiHost<config::ADS1299::Config> const& bus);

		void Init();
		void Handler();
		bool IsReady() const;

		mem::ring_buffer_t RingBuffer() const;

	private:

		using voltage_t = int32_t;

		enum class State : util::byte;
		struct Command;
		struct Register;
		struct RegisterConfigFlag;
		struct Config1Flags;
		struct Config2Flags;
		struct Config3Flags;
		struct ChannelFlags;

		struct RegisterAccess
		{
			enum : util::byte
			{
				Read = 0x20,
				Write = 0x40,
			};
		};
		static constexpr util::byte RREG(util::byte registerAddress);
		static constexpr util::byte WREG(util::byte registerAddress);

		void CaptureData();
		void CaptureNoiseData();

		void Reset();
		void PowerUp();
		void WaitForPower();
		void ConfigureExternalReference();
		bool IsPoweredUp();
		void SetTestSignals();
		void SetNoiseData();
		void SetCustomSettings();

		State              _state;
		voltage_t          _noise[config::ADS1299::CHANNEL_COUNT]{}; // Noise data
		voltage_t          _ecg[config::ADS1299::CHANNEL_COUNT]{};   // Electrocardiography data
		uint32_t           _statusBits{};
		size_t             _resetCounter{};
		mem::ring_buffer_t _buffer;
		StaticSemaphore_t  _mutexBuffer;
	};

	constexpr util::byte ADS1299::RREG(util::byte registerAddress)
	{
		return registerAddress | RegisterAccess::Read;
	}

	constexpr util::byte ADS1299::WREG(util::byte registerAddress)
	{
		return registerAddress | RegisterAccess::Write;
	}
}
