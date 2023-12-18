#pragma once

// internal
#include "../config/devices.h"
#include "../util/types.h"
#include "../memory/int.h"
#include "../memory/ring_buffer.h"
// external
#include <esp_util/spiDevice.hpp>
#include <esp_util/spiHost.hpp>
#include "freertos/FreeRTOS.h"
#include "esp_attr.h"
// std
#include <chrono>

namespace device
{
	class ADS1299 : protected esp::spiDevice<config::ADS1299::Config, config::ADS1299::SPI_MAX_TRANSACTION_LENGTH>
	{
	public:
		explicit ADS1299(esp::spiHost<config::ADS1299::Config> const& bus);

		void Init();
		void IRAM_ATTR Handler();
		bool IsReady() const;

		mem::RingBuffer* ECGRingBuffer();
		mem::RingBuffer* NoiseRingBuffer();
	private:
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
		using timepoint_t = std::chrono::time_point<std::chrono::system_clock>;
		using voltage_t = mem::int24_t;
		using status_t  = mem::int24_t;

		struct ecg_t
		{
			voltage_t channels[config::ADS1299::CHANNEL_COUNT];
		};

		static constexpr util::byte RREG(util::byte registerAddress);
		static constexpr util::byte WREG(util::byte registerAddress);

		void CaptureData();

		void Reset();
		void PowerUp();
		void WaitForPower();
		void ConfigureExternalReference();
		bool IsPoweredUp();
		void SetTestSignals();
		void SetNoiseData();
		void SetCustomSettings();

		State _state;
		ecg_t _noise[config::ADS1299::NOISE_SAMPLES_IN_RING_BUFFER]; // Noise data (Not implemented)
		ecg_t _ecg[config::ADS1299::ECG_SAMPLES_IN_RING_BUFFER]; // Electrocardiography data
		timepoint_t       _nextTime;
		uint32_t          _statusBits;
		size_t            _resetCounter;
		mem::RingBuffer	  _ecgBuffer;
		mem::RingBuffer	  _noiseBuffer;
		StaticSemaphore_t _mutexBuffer[2];
	};

	constexpr util::byte ADS1299::RREG(util::byte registerAddress)
	{
		return registerAddress | ADS1299::RegisterAccess::Read;
	}

	constexpr util::byte ADS1299::WREG(util::byte registerAddress)
	{
		return registerAddress | ADS1299::RegisterAccess::Write;
	}
}
