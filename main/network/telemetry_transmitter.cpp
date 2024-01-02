#include "telemetry_transmitter.h"

#include <cassert>
#include <chrono>

#include "../memory/int.h"
#include "../config/devices.h"
#include "../config/task.h"
#include "bdf_plus.h"
#include "../memory/stack.h"
#include "../util/utils.h"

#include <cstdio>
#include <freertos/FreeRTOS.h>
#include "esp_timer.h"


#define PORT          1212
#define TELEMETRY_TAG "[TelemetryTask:]"

namespace net
{
	mem::int24_t               gSendStackBuffer[config::BDF::SEND_STACK_SIZE];
	mem::Stack::layout_section gSendStackLayout[config::BDF::OVERALL_CHANNELS];

	TelemetryTransmitter::TelemetryTransmitter(mem::RingBufferView const* view)
		: _bufferView(*view), _sendStack(mem::Stack(gSendStackBuffer, gSendStackLayout)), _socket(PORT), _channelCount(0), _stackSize(0)
	{
		for(auto const& buffer : _bufferView)
		{
			for(util::size_t channel = 0; channel < buffer->ChannelCount(); ++channel, ++_channelCount)
			{
				const mem::Stack::size_type sectionSize = buffer->NodesInBDFRecord() * sizeof(mem::int24_t);
				gSendStackLayout[_channelCount] = mem::Stack::layout_section{.level = 0, .size = sectionSize, .off = _stackSize};
				_stackSize += sectionSize;
			}	
		}
	}

	void TelemetryTransmitter::TryAgain()
	{
		PRINTI("[Socket:]", "Closing socket\n");
		_socket.Close();
		_socket.Open();
	}

	bool TelemetryTransmitter::FindServer()
	{
		if(_socket.IsConnected()) return true;

		if(_socket.Open() != net::TCPError::NO_ERROR)
		{
			PRINTI(TELEMETRY_TAG, "Unable to open tcp client socket: %s\n", strerror(errno));
			return false;
		}
		_socket.SetTimeout(1, 0);

		const char* serverAddress = "192.168.2.106";

		// Try to connect to server
		PRINTI(TELEMETRY_TAG, "Waiting for device discover broadcast.\n");
		while(_socket.Connect(serverAddress) == net::TCPError::CONNECTING_FAILED)
		{
			YIELD_FOR(5'000); // Try to find server only every 5 seconds
		}
		PRINTI(TELEMETRY_TAG, "Established TCP connection to server.\n");
		return true;
	}

	bool TelemetryTransmitter::SendHeaders() 
	{
		file::bdf_header_t generalHeader{};
		file::create_general_header(&generalHeader, 
									config::DURATION_OF_MEASUREMENT, 
									-1, 
									_channelCount);

		// Receive header request and send it
		_socket.WaitFor(file::BDF_COMMANDS::REQ_HEADER);
		PRINTI(TELEMETRY_TAG, "Received header request.\n");
		if(_socket.Send(&generalHeader, sizeof(generalHeader)) == net::TCPError::SENDING_FAILED)
		{
			return false;
		}
		PRINTI(TELEMETRY_TAG, "Send header.\n");

		// Receive record header request. Send all channel headers of all RingBuffer.
		_socket.WaitFor(file::BDF_COMMANDS::REQ_RECORD_HEADERS);
		PRINTI(TELEMETRY_TAG, "Received record header request.\n");

		// Send all record headers in a per attribute manner
#define TARGET_BDF_HEADER_MEMBER(type, member) offsetof(type, member), sizeof type::member
#define TARGET_BDF_MEMBER(member)              TARGET_BDF_HEADER_MEMBER(file::bdf_signal_header_t, member)
		SendHeadersAttribute(TARGET_BDF_MEMBER(label));
		SendHeadersAttribute(TARGET_BDF_MEMBER(transducer_type));
		SendHeadersAttribute(TARGET_BDF_MEMBER(physical_dimension));
		SendHeadersAttribute(TARGET_BDF_MEMBER(physical_minimum));
		SendHeadersAttribute(TARGET_BDF_MEMBER(physical_maximum));
		SendHeadersAttribute(TARGET_BDF_MEMBER(digital_minimum));
		SendHeadersAttribute(TARGET_BDF_MEMBER(digital_maximum));
		SendHeadersAttribute(TARGET_BDF_MEMBER(pre_filtering));
		SendHeadersAttribute(TARGET_BDF_MEMBER(nr_of_samples_in_signal));
		SendHeadersAttribute(TARGET_BDF_MEMBER(reserved));

		return true;
	}

	long TelemetryTransmitter::GetNumberOfMeasurements()
	{
		_socket.SetTimeout(60, 0);
		char      reqRecordsBuffer[file::BDF_COMMANDS::REQ_RECORDS.size() + 1 + 6 + 1];
		const int received = _socket.Receive(reqRecordsBuffer, std::size(reqRecordsBuffer) - 1);
		if(received <= 0)
		{
			PRINTI(TELEMETRY_TAG, "Error received no request.\n");
			return -1;
		}
		reqRecordsBuffer[received] = '\0';
		const int isDifferent = std::memcmp(reqRecordsBuffer, file::BDF_COMMANDS::REQ_RECORDS.data(), file::BDF_COMMANDS::REQ_RECORDS.size());
		if(isDifferent)
		{
			PRINTI(TELEMETRY_TAG, "Error received invalid request.\n");
			return -1;
		}
		return std::strtol(reqRecordsBuffer + file::BDF_COMMANDS::REQ_RECORDS.size() + 1, nullptr, 10);
	}

	void TelemetryTransmitter::BeginTransmission(long const& numberOfMeasurements)
	{
		xEventGroupSetBits(config::SensorControlEventGroup, SensorControlEvent::StartMeasurement);
		// Send records
		unsigned written = 0;
		PRINTI(TELEMETRY_TAG, "Sending %ld data records.\n", numberOfMeasurements);
		for(int measurement = 0; measurement < numberOfMeasurements; measurement++)
		{

			written += SendDataRecord();
			PRINTI(TELEMETRY_TAG, "Send a data record.\n");

		}
		xEventGroupSetBits(config::SensorControlEventGroup, SensorControlEvent::StopMeasurement);
		PRINTI(TELEMETRY_TAG, "Written %u bytes of data records to server\n", written);
	}

	void TelemetryTransmitter::BeginTransmission()
	{
		_socket.SetTimeout(2, 0);
		//_sendStack.Clear();
		//gView.ResetAll();
		do
		{
			SendDataRecord();
			PRINTI(TELEMETRY_TAG, "Send a data record.\n");
		}
		while(!_socket.Check(file::BDF_COMMANDS::REQ_STOP));
	}

	void TelemetryTransmitter::SendHeadersAttribute(size_type const& attributeOffset, size_type const& attributeSize) 
	{
		for(auto const& buffer : _bufferView)
		{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
			for(auto channel = 0; channel < buffer->ChannelCount(); ++channel)
				auto _ = _socket.Send(static_cast<void const*>(buffer->RecordHeaders() + channel) + attributeOffset, attributeSize);
#pragma GCC diagnostic pop
		}

		//for(auto const& b : gView)
		//{
		//	const auto header = b->RecordHeaders();
		//	for(uint32_t channel = 0; channel < b->ChannelCount(); channel++)
		//	{
		//		auto _ = _socket.Send(reinterpret_cast<util::byte const*>(header + channel) + attributeOffset, attributeSize);
		//	}
		//}
	}

	TelemetryTransmitter::size_type IRAM_ATTR TelemetryTransmitter::SendDataRecord() 
	{
		mem::Stack::size_type channel;
		static uint32_t transmitter_count = 0;
		uint64_t start = esp_timer_get_time();
		do
		{
			channel = 0;
			for(mem::RingBuffer* buffer: _bufferView)
			{
				int fits = _sendStack.Fits(channel, sizeof(mem::int24_t));
				int hasData = buffer->HasData();
				while(fits && hasData)
				{
					//buffer->Lock();
					void const* data = buffer->CurrentRead();
					_sendStack.PushNChannels(data, sizeof(mem::int24_t), channel, buffer->ChannelCount());
					buffer->ReadAdvance(1);
					//buffer->Unlock();
				}
				if(transmitter_count++ % 100 == 0)
					PRINTI("[Transmitter:]", "transmitter fits=%d, hasData=%d\n", fits, hasData);
				channel += buffer->ChannelCount();
			}
			YIELD_FOR(20);
		}
		while (!_sendStack.Full(0, channel));
		uint64_t end = esp_timer_get_time();
		printf("Write took %llu milliseconds (%llu microseconds)\n", (end - start) / 1000, (end - start));
		TCPError error = _socket.Send(_sendStack.Data(), _stackSize);
		
		_sendStack.Clear();
		return _stackSize;
	}
}

