#include "telemetry_transmitter.h"

#include "../memory/ring_buffer.h"
#include "fmt/format.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../config/devices.h"

#include "../network/wifi.hpp"
#include "../network/sockets.h"

#define TEST_SENSORS 0

namespace sys
{
	void telemetry_transmitter_task(void* args)
	{
		mem::ring_buffer_t** ringBuffers;
		size_t				 sensorCount;

		// Get infos about ring buffers.
		{
			auto*** ringBufferInfoPtrPtr = static_cast<mem::ring_buffer_t***>(args);
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

#if TEST_SENSORS == 1
		// Establish wifi connection
		const char* ssid            = "WLAN-Q3Q83P_EXT";
		const char* pw              = "1115344978197496";
		net::connect(ssid, pw);
		net::wait_for_connection();

		// Open UDP Socket
		auto socket = net::open_udp_socket();
		if(!net::is_valid(socket))
		{
			fmt::print("[TelemetryTransmitterTask:] Unable to open UDP socket.");
			return;
		}
		net::get_ip_info();

		net::client_t client = net::INVALID_CLIENT;

		if(!is_valid(client = net::check_for_clients(socket)))
		{
			return;
		}
		
		net::send_ok(socket, client);

#endif
		printf("HHH\n");


		while(true)
		{
			if(mem::hasData(*ringBuffers))
			{
				auto sample     = mem::read<sample_t>(*ringBuffers);
				fmt::print("Red = {}, Infrared = {}\n", sample.red, sample.infraRed);
			}

			if(mem::hasData(*(ringBuffers + 1)))
			{
				auto ecg = mem::read<ecg_t>(*(ringBuffers + 1));
				fmt::print("Ch1 = {}, Ch2 = {}, Ch3 = {}\n", ecg.channel[0], ecg.channel[1], ecg.channel[2]);
			}

			if(mem::hasData(*(ringBuffers + 2)))
			{
				auto acceleration = mem::read<acceleration_t>(*(ringBuffers + 2));
				fmt::print("X = {}, Y = {}, Z = {}, status = {} \n", acceleration.X, acceleration.Y, acceleration.Z, acceleration.status);
			}

			//auto voltage    = mem::read<voltage_t>(ringBuffers + 1);
			//fmt::print("Red = {}, Infrared = {}, Voltage = {} \n", sample.red, sample.infraRed, voltage);
			//fmt::print("X = {}, Y = {}, Z = {}, status = {} \n", acceleration.X, acceleration.Y, acceleration.Z, acceleration.status);
			vTaskDelay(pdMS_TO_TICKS(100));
		}
	}
}