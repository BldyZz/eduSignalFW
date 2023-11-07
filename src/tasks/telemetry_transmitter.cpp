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
		net::Socket socket;
		constexpr static net::port_t port = 1212;
		socket.Open(net::Protocol::TCP, port);
		socket.AutoConnect();

		char okMsg[] = "OK";
		socket.Send(okMsg, std::size(okMsg));
		//char rcv[std::size(okMsg)];
		//socket.Receive(rcv, std::size());

		while (true)
		{
		}

		uint16_t channelsN = 0;
		for(uint16_t rbuf = 0; rbuf < ringBufferArray.size; rbuf++)
		{
			channelsN += ringBufferArray.buffers[rbuf]->ChannelCount();
		}

		file::bdf_header_t generalHeader;
		file::create_general_header(&generalHeader, 0.03125f, channelsN);
		socket.Send(&generalHeader, sizeof(generalHeader));

		for(uint16_t rbuf = 0; rbuf < ringBufferArray.size; rbuf++)
		{
			file::bdf_record_header_t record_header;
			file::create_record_header(&record_header, "", "", "", -2, 2, -1, 1, "", 0);
			socket.Send(&record_header, sizeof(record_header));
		}

		util::byte send_buffer[1024];
		mem::Stack send_stack = send_buffer;
		
		while(true)
		{
			for(util::byte i = 0; i < ringBufferArray.size; i++)
			{
				if(ringBufferArray.buffers[i]->HasData())
				{
					const bool fits = send_stack.Fits(ringBufferArray.buffers[i]->NodeSize());

					if(!fits) socket.Send(send_stack.Data(), send_stack.Size());

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