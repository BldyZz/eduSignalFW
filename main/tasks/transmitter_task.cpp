#include "transmitter_task.h"

#include <cassert>

#include "../config/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../memory/ring_buffer.h"
#include "../util/time.h"

#include "../network/wifi.hpp"
#include "../network/bdf_plus.h"
#include "../network/tcp_client.h"
#include "../network/telemetry_transmitter.h"
#include "../memory/nvs.h"


#define TELEMETRY_TAG "[TelemetryTask:]"

namespace mem
{
	struct int24_t;
}

/**
 * \brief Forward declarations
 */
namespace sys
{
	void send_data(mem::RingBufferView const& view, net::TCPClient const& socket);
}

/**
 * \brief Definitions
 */
namespace sys
{
	long GetDataRecordCountFromClient(net::TCPClient const& client);

	void transmitter_task(void* view)
	{
		esp_util::nvs_init();
		// Establish wifi connection
		char const* ssid = "WLAN-Q3Q83P_EXT";
		char const* pw   = "1115344978197496";
		net::connect(ssid, pw);
		net::wait_for_connection();
		net::print_ip_info();
		const mem::RingBufferView ringBufferView = *static_cast<mem::RingBufferView*>(view);
		*static_cast<mem::RingBufferView*>(view) = mem::RingBufferView();
		net::TelemetryTransmitter telemetry(&ringBufferView);
		while (true)
		{
			if(!telemetry.FindServer())
			{
				telemetry.TryAgain();
				continue;
			}
			if(!telemetry.SendHeaders())
			{
				telemetry.TryAgain();
				continue;
			}
			const long numberOfMeasurements = telemetry.GetNumberOfMeasurements();
			if(numberOfMeasurements > 0)
			{
				telemetry.BeginTransmission(numberOfMeasurements);
			}
			else if(numberOfMeasurements == 0)
			{
				telemetry.BeginTransmission();
			}
			else
			{
				telemetry.TryAgain();
				continue;
			}
			// else continue
			YIELD_FOR(5'000);
		}

		// Open TCP client socket
		//	constexpr static net::port_t PORT = 1212;
		//	net::TCPClient client(PORT);
		//	
		//	if(client.Open() != net::TCPError::NO_ERROR)
		//	{
		//		PRINTI(TELEMETRY_TAG, "Unable to open tcp client socket: %s\n", strerror(errno));
		//		return;
		//	}
		//	
		//	
		//	
		//	
		//	
		//	file::bdf_header_t generalHeader{};
		//	file::create_general_header(&generalHeader, config::DURATION_OF_MEASUREMENT, -1, channelsN);
		//	
		//	const char* serverAddress = "192.168.2.106";
		//	
		//	// Connect to server
		//	PRINTI(TELEMETRY_TAG, "Waiting for device discover broadcast.\n");
		//	while(client.Connect(serverAddress) == net::TCPError::CONNECTING_FAILED)
		//	{
		//		YIELD_FOR(5'000); // Try to find server only every 5 seconds
		//	}
		//	PRINTI(TELEMETRY_TAG, "Established TCP connection to server.\n");
		//	
		//	while(true)
		//	{
		//		// Receive header request and send it
		//		client.WaitFor(file::BDF_COMMANDS::REQ_HEADER);
		//		PRINTI(TELEMETRY_TAG, "Received header request.\n");
		//		if(client.Send(&generalHeader, sizeof(generalHeader)) == net::TCPError::SENDING_FAILED)
		//		{
		//			client.Close();
		//			client.Open();
		//			continue;
		//		}
		//		PRINTI(TELEMETRY_TAG, "Send header.\n");
		//	
		//		// Receive record header request. Send all channel headers of all RingBuffer.
		//		client.WaitFor(file::BDF_COMMANDS::REQ_RECORD_HEADERS);
		//		PRINTI(TELEMETRY_TAG, "Received record header request.\n");
		//	
		//		send_record_headers(ringBufferView, client);
		//	
		//		// Receive a command to start sending record data
		//		const long numberOfMeasurements = GetDataRecordCountFromClient(client);
		//		if(numberOfMeasurements <= 0) continue;
		//	
		//		if(numberOfMeasurements < 0)
		//		{
		//			// indefinite condition
		//			
		//		} else if(numberOfMeasurements > 0)
		//		{
		//			
		//		}
		//		PRINTI(TELEMETRY_TAG, "Send all data records.\n");
		//	}
	}

//	void send_data(mem::RingBufferView const& view, net::TCPClient const& socket)
//	{
//		unsigned written = 0;
//		for(auto const& b : view)
//		{
//			const auto nodesInBDFRecord = b->NodesInBDFRecord();
//			const auto nodeSize = b->NodeSize();
//			const auto size = nodesInBDFRecord * nodeSize;
//			assert(nodeSize == 3);
//			assert(nodesInBDFRecord <= 60);
//			while(true)
//			{
//				if(b->Size() >= nodesInBDFRecord)
//				{
//					b->Lock();
//					if(b->IsOverflowing())
//					{
//						const auto nodesToOverflow = b->NodesToOverflow();
//						const auto nodesAfterOverflow = nodesInBDFRecord - nodesToOverflow;
//						assert(nodesToOverflow + nodesAfterOverflow == nodesInBDFRecord);
//						void* readToOverflow = b->ReadAdvance(nodesToOverflow);
//						void* readAfterOverflow = b->ReadAdvance(nodesAfterOverflow);
//						const auto nodesToOverflowSize = nodesToOverflow * nodeSize;
//						const auto nodesAfterOverflowSize = nodesAfterOverflow * nodeSize;
//						unsigned numberOfChannels = b->ChannelCount();
//						for(auto channel = 0; channel < numberOfChannels; channel++)
//						{
//							socket.Send(b->ChangeChannel(readToOverflow, channel), nodesToOverflowSize);
//							socket.Send(b->ChangeChannel(readAfterOverflow, channel), nodesAfterOverflow);
//						}
//						written += nodesToOverflowSize + nodesAfterOverflowSize;
//						//PRINTI(TELEMETRY_TAG, "before: %u after: %u\n", nodesToOverflowSize, nodesAfterOverflow);
//					} 
//					else
//					{
//						void* read = b->ReadAdvance(nodesInBDFRecord);
//						for(auto channel = 0; channel < b->ChannelCount(); channel++)
//						{
//							socket.Send(b->ChangeChannel(read, channel), size);
//						}
//						written += size;
//						//PRINTI(TELEMETRY_TAG, "overall: %u", size);
//					}
//					b->Unlock();
//					break;
//				}
//			}
//		}
//		PRINTI(TELEMETRY_TAG, "Written %u Bytes\n", written);
//	}
//
//#define TARGET_BDF_HEADER_MEMBER(type, member) offsetof(type, member), sizeof type::member
//#define TARGET_BDF_MEMBER(member)              TARGET_BDF_HEADER_MEMBER(file::bdf_signal_header_t, member)
//
//	
//	long GetDataRecordCountFromClient(net::TCPClient const& client)
//	{
//		return -1;
//	}
}
