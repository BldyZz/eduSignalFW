#include "telemetry_transmitter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../memory/ring_buffer.h"
#include "../config/devices.h"
#include "../util/time.h"

#include "../network/wifi.hpp"
#include "../network/sockets.h"
#include "../network/bdf_plus.h"

#include "task_config.h"

#include <cmath>

#define TEST_SENSORS 0

namespace sys
{
	void send_data(mem::RingBufferView const& view, net::Socket& socket);

	void telemetry_transmitter_task(void* array)
	{
		// Get infos about ring buffers.
		mem::RingBufferView ringBufferView = *static_cast<mem::RingBufferView*>(array);

		// Establish wifi connection
		const char* ssid            = "WLAN-Q3Q83P_EXT";
		const char* pw              = "1115344978197496";
		net::connect(ssid, pw);
		net::wait_for_connection();

		
		// Open UDP Socket
		net::Socket socket;
		constexpr static net::port_t port = 1212;
		socket.Open(net::Protocol::TCP, port);

		uint16_t channelsN = 0;
		for(auto const& b : ringBufferView)
		{
			channelsN += b->ChannelCount();
		}

		file::bdf_header_t generalHeader{};
		file::create_general_header(&generalHeader, config::DURATION_OF_MEASUREMENT, channelsN);

		while (true)
		{
			// Connection stage
			PRINTI("[TelemetryTask:]", "Waiting for device discover broadcast.\n");
			socket.AutoConnect(file::BDF_COMMANDS::DISCOVER);

			// Send ACKNOWLEDGE
			socket.Send(file::BDF_COMMANDS::ACKNOWLEDGE);
			PRINTI("[TelemetryTask:]", "Send acknowledgment.\n");

			// Receive header request and send it
			socket.ReceiveAndCompareIndefinite(file::BDF_COMMANDS::REQ_HEADER);
			PRINTI("[TelemetryTask:]", "Received header request.\n");
			socket.Send(&generalHeader, sizeof(generalHeader));
			PRINTI("[TelemetryTask:]", "Send header.\n");

			// Receive record header request. Send all channel headers of all RingBuffer.
			socket.ReceiveAndCompareIndefinite(file::BDF_COMMANDS::REQ_RECORD_HEADERS);
			PRINTI("[TelemetryTask:]", "Received record header request.\n");
			for(auto const& b : ringBufferView)
			{
				const auto header = b->RecordHeaders();
				for(uint32_t channel = 0; channel < b->ChannelCount(); channel++)
				{
					socket.Send(header + channel, sizeof(file::bdf_record_header_t));
				}
			}
			PRINTI("[TelemetryTask:]", "Send data record headers.\n");

			// Receive a command to start sending record data
			float time;
			{
				char      reqRecordsBuffer[file::BDF_COMMANDS::REQ_RECORDS.size() + 1 + 6 + 1];
				const int received = socket.Receive(reqRecordsBuffer, std::size(reqRecordsBuffer) - 1);
				if(received <= 0)
				{
					PRINTI("[TelemetryTask:]", "Error received no request.\n");
					continue;
				}
				reqRecordsBuffer[received] = '\0';
				int isSame = std::memcmp(reqRecordsBuffer, file::BDF_COMMANDS::REQ_RECORDS.data(), file::BDF_COMMANDS::REQ_RECORDS.size());
				// TODO if(!isSame) // Error Handling{ }
				time = std::strtof(reqRecordsBuffer + file::BDF_COMMANDS::REQ_RECORDS.size() + 1, nullptr);
			}

			
			if(time == 0.f)
			{
				// indefinite condition
				socket.SetTimeout(0, 1'000);
				do
				{
					send_data(ringBufferView, socket);
					WATCHDOG_HANDLING();
				} while(socket.ReceiveAndCompare(file::BDF_COMMANDS::REQ_STOP));
			}
			else
			{
				const int numberOfMeasurements = std::round(time / config::DURATION_OF_MEASUREMENT);
				
				// Send records
				for(int measurement = 0; measurement < numberOfMeasurements; measurement++)
				{
					send_data(ringBufferView, socket);
					WATCHDOG_HANDLING();
				}
			}
			
		}
	}

	void send_data(mem::RingBufferView const& view, net::Socket& socket)
	{
		for(auto const& b : view)
		{
			const auto nodesInBDFRecord = b->NodesInBDFRecord();

			while (true)
			{
				if(b->Size() >= nodesInBDFRecord)
				{
					b->Lock();
					if(b->IsOverflowing())
					{
						const auto nodesToOverflow = b->NodesToOverflow();
						const auto nodesAfterOverflow = nodesInBDFRecord - nodesToOverflow;
						void* readToOverflow    = b->ReadAdvance(nodesToOverflow);
						void* readAfterOverflow = b->ReadAdvance(nodesAfterOverflow);
						for(auto channel = 0; channel < b->ChannelCount(); channel++)
						{
							socket.Send(b->ChangeChannel(readToOverflow, channel), nodesToOverflow * b->NodeSize());
							socket.Send(b->ChangeChannel(readAfterOverflow, channel), nodesAfterOverflow * b->NodeSize());
						}
					}
					else
					{
						void* read = b->ReadAdvance(nodesInBDFRecord);
						for(auto channel = 0; channel < b->ChannelCount(); channel++)
						{
							socket.Send(b->ChangeChannel(read, channel), nodesInBDFRecord * b->NodeSize());
						}
					}
					b->Unlock();
					break;
				}
			}
		}
	}
}