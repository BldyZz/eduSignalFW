#pragma once

// external
#include "esp_util/i2cDevice.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
// internal
#include "../config/devices.h"
#include "../util/types.h"
#include "../memory/ring_buffer.h"
// std
#include <span>

namespace device
{
	class BHI160 : esp::i2cDevice<config::BHI160::Config, config::BHI160::ADDRESS>
	{
	public:
		BHI160();

		void                Init();
		void                Handler();
		bool                IsReady() const;
		mem::ring_buffer_t* RingBuffer();
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
		void GetRemainingFIFOSize();
		void GetData();
		void PrintVersionAndStatus();

		struct acceleration_t
		{
			int16_t X, Y, Z;
			util::byte status;
		};

		acceleration_t _acceleration[10];

		util::timestamp_t  _timestamp;
		std::uint16_t      _bytesInFIFO;
		State              _state;
		StaticSemaphore_t  _mutexBuffer{};
		mem::ring_buffer_t _buffer;
	};
}