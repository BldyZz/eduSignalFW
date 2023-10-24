#include "telemetry_transmitter.h"

#include "../memory/ring_buffer.h"
#include "fmt/format.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../config/devices.h"

namespace sys
{
	void telemetry_transmitter_task(void* args)
	{
		mem::ring_buffer_t* ringBuffers;
		size_t				sensorCount;

		// Get infos about ring buffers.
		{
			auto** ringBufferInfoPtrPtr = static_cast<mem::ring_buffer_t**>(args);
			ringBuffers = *ringBufferInfoPtrPtr;
			sensorCount = *reinterpret_cast<size_t*>(ringBufferInfoPtrPtr + 1);
			*ringBufferInfoPtrPtr = nullptr; // Ready Signal
		}



		struct sample_t
		{
			uint32_t red;
			uint32_t infraRed;
		};
		struct acceleration_t
		{
			int16_t X, Y, Z;
			util::byte status;
		};

		using voltage_t = int32_t;
		struct ecg_t
		{
			voltage_t channel[config::ADS1299::CHANNEL_COUNT] = {};
		};


		while(true)
		{
			//auto ecg = mem::read<ecg_t>(ringBuffers);
			//fmt::print("Ch1 = {}, Ch2 = {}, Ch3 = {}\n", ecg.channel[0], ecg.channel[1], ecg.channel[2]);

			//auto sample     = mem::read<sample_t>(ringBuffers);
			//fmt::print("Red = {}, Infrared = {}\n", sample.red, sample.infraRed);

			auto acceleration = mem::read<acceleration_t>(ringBuffers);
			fmt::print("X = {}, Y = {}, Z = {}, status = {} \n", acceleration.X, acceleration.Y, acceleration.Z, acceleration.status);

			//auto voltage    = mem::read<voltage_t>(ringBuffers + 1);
			//fmt::print("Red = {}, Infrared = {}, Voltage = {} \n", sample.red, sample.infraRed, voltage);
			//fmt::print("X = {}, Y = {}, Z = {}, status = {} \n", acceleration.X, acceleration.Y, acceleration.Z, acceleration.status);
			vTaskDelay(pdMS_TO_TICKS(100));
		}
	}
}