#include "telemetry_transmitter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../memory/ring_buffer.h"
#include "../memory/stack.h"
#include "../config/devices.h"
#include "../util/time.h"

#include "../network/wifi.hpp"
#include "../network/sockets.h"
#include "../network/bdf_plus.h"

#include "task_config.h"

#define TEST_SENSORS 0

namespace sys
{
	void telemetry_transmitter_task(void* array)
	{
		// Get infos about ring buffers.
		mem::RingBufferArray ringBufferArray = *static_cast<mem::RingBufferArray*>(array);

		// Establish wifi connection
		const char* ssid            = "WLAN-Q3Q83P_EXT";
		const char* pw              = "1115344978197496";
		net::connect(ssid, pw);
		net::wait_for_connection();

		// Open UDP Socket
		auto socket = net::open_udp_socket();
		if(!net::is_valid(socket))
		{
			std::printf("[TelemetryTransmitterTask:] Unable to open UDP socket.");
			return;
		}
		net::get_ip_info();

		net::client_t client;
		if(!is_valid(client = net::check_for_clients(socket)))
		{
			return;
		}
		
		net::send_ok(socket, client);

		util::byte send_buffer[1024];
		mem::Stack send_stack = send_buffer;
		
		while(true)
		{
			for(util::byte i = 0; i < ringBufferArray.size; i++)
			{
				if(ringBufferArray.buffers[i]->HasData())
				{
					const bool fits = send_stack.Fits(ringBufferArray.buffers[i]->NodeSize());

					if(!fits) net::send_pkg(socket, client, send_stack.Data(), send_stack.Size());

					ringBufferArray.buffers[i]->Lock();
					send_stack.Push(ringBufferArray.buffers[i]->ReadAdvance(), ringBufferArray.buffers[i]->NodeSize());
					ringBufferArray.buffers[i]->Unlock();
				}
			}
			WATCHDOG_HANDLING();
			//	
			//	if(ringBuffers[0]->HasData())
			//	{
			//		auto sample     = ringBuffers[0]->read<sample_t>();
			//		fmt::print("Red = {}, Infrared = {}\n", sample.red, sample.infraRed);
			//	}
			//	
			//	if(ringBuffers[1]->HasData())
			//	{
			//		auto ecg = ringBuffers[1]->read<ecg_t>();
			//		fmt::print("Ch1 = {}, Ch2 = {}, Ch3 = {}\n", ecg.channel[0], ecg.channel[1], ecg.channel[2]);
			//	}
			//	
			//	if(ringBuffers[2]->HasData())
			//	{
			//		auto acceleration = mem::read<acceleration_t>(*(ringBuffers + 2));
			//		fmt::print("X = {}, Y = {}, Z = {}, status = {} \n", acceleration.X, acceleration.Y, acceleration.Z, acceleration.status);
			//	}

			//auto voltage    = mem::read<voltage_t>(ringBuffers + 1);
			//fmt::print("Red = {}, Infrared = {}, Voltage = {} \n", sample.red, sample.infraRed, voltage);
			//fmt::print("X = {}, Y = {}, Z = {}, status = {} \n", acceleration.X, acceleration.Y, acceleration.Z, acceleration.status);
			//vTaskDelay(pdMS_TO_TICKS(100));
		}
	}
}