#pragma once

// external
#include "esp_util/i2cDevice.hpp"
#include "freertos/FreeRTOS.h"
#include "esp_attr.h"
// internal
#include "../config/devices.h"
#include "../util/types.h"
#include "../memory/ring_buffer.h"
#include "../memory/int.h"
// std
#include <span>

namespace device
{
	class BHI160 : esp::i2cDevice<config::BHI160::Config, config::BHI160::ADDRESS>
	{
	public:
		BHI160();

		void             Init();
		bool             IsReady() const;
		mem::RingBuffer* RingBuffer();
		bool             HasData() const;
		void             GetRemainingFIFOSize();
		void             GetData();
		void             InsertPadding();

	private:

		enum class State : util::byte;
		struct Command;
		struct Register;
		struct Event;

		void HandleData(std::span<util::byte> package);
		void Reset();
		void StartRAMPatch();
		void UploadFirmware();
		void StartCPU();
		void ConfigureDevices();
		void PrintVersionAndStatus();

		using acceleration_storage_t = mem::int24_t;
		struct acceleration_t
		{
			acceleration_storage_t X, Y, Z;
			acceleration_storage_t status;
		};

		using timepoint_t = std::chrono::time_point<std::chrono::system_clock>;
		acceleration_t _acceleration[config::BHI160::SAMPLES_IN_RING_BUFFER];

		util::timestamp_t         _timestamp;
		timepoint_t               _nextTime;
		std::uint16_t             _bytesInFIFO;
		State                     _state;
		StaticSemaphore_t         _mutexBuffer{};
		mem::RingBuffer           _buffer;
	};
}