#include "telemetry_transmitter.h"

#include <cassert>
#include <chrono>

#include "../memory/int.h"
#include "../config/devices.h"
#include "../tasks/task_config.h"
#include "bdf_plus.h"
#include "../memory/stack.h"

#include <cstdio>
#include <freertos/FreeRTOS.h>
#include "esp_timer.h"


#define PORT          1212
#define TELEMETRY_TAG "[TelemetryTask:]"
#define TRANSMISSION_ERROR(x) if((x) != net::TCPError::NO_ERROR) return false;

namespace sys
{
	extern mem::SensorView<mem::int24_t> gSensorView;
}

namespace net
{
	file::bdf_signal_header_t  gSignalHeaders[config::BDF::OVERALL_CHANNELS];
	mem::int24_t               gSendStackBuffer[config::BDF::SEND_STACK_SIZE];
	mem::Stack::layout_section gSendStackLayout[config::BDF::OVERALL_CHANNELS];
	

	TelemetryTransmitter::TelemetryTransmitter()
		: _sendStack(mem::Stack(gSendStackBuffer, gSendStackLayout)), _socket(PORT), _channelCount(0), _stackSize(0)
	{
		while(sys::gSensorView.empty()) { YIELD_FOR(50); }
		_sensorView = sys::gSensorView;

		// Create BDF Headers
		std::size_t off = 0;
		off = file::createBDFHeader<config::ADS1299>(gSignalHeaders, off);
		off = file::createBDFHeader<config::MAX30102>(gSignalHeaders, off);
		off = file::createBDFHeader<config::BHI160>(gSignalHeaders, off);

		for(auto const& sensor : _sensorView)
		{
			for(util::size_t channel = 0; channel < sensor.size(); ++channel, ++_channelCount)
			{
				const mem::Stack::size_type sectionSize = gSignalHeaders[_channelCount].samples_in_bdf_record * sizeof(mem::int24_t);
				gSendStackLayout[_channelCount] = mem::Stack::layout_section{.level = 0, .size = sectionSize, .off = _stackSize};
				_stackSize += sectionSize;
			}	
		}
	}

	bool TelemetryTransmitter::FindServer()
	{
		if(_socket.IsConnected()) return true;
		YIELD_FOR(500);
		if(_socket.Open() != net::TCPError::NO_ERROR)
		{
			PRINTI(TELEMETRY_TAG, "Unable to open tcp client socket: %s\n", strerror(errno));
			return false;
		}
		_socket.SetTimeout(5, 0);

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
		TRANSMISSION_ERROR(_socket.WaitFor(file::BDF_COMMANDS::REQ_HEADER));
		PRINTI(TELEMETRY_TAG, "Received header request.\n");

		TRANSMISSION_ERROR(_socket.Send(&generalHeader, sizeof(generalHeader)));
		PRINTI(TELEMETRY_TAG, "Sent header.\n");

		// Receive record header request. Send all channel gSignalHeaders of all RingBuffer.
		TRANSMISSION_ERROR(_socket.WaitFor(file::BDF_COMMANDS::REQ_RECORD_HEADERS));
		PRINTI(TELEMETRY_TAG, "Received record header request.\n");
		
		// Send all record gSignalHeaders in a per attribute manner
#define TARGET_BDF_HEADER_MEMBER(type, member) offsetof(type, member), sizeof type::member
#define TARGET_BDF_MEMBER(member)              TARGET_BDF_HEADER_MEMBER(file::bdf_signal_header_t, member)
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(label)));
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(transducer_type)));
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(physical_dimension)));
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(physical_minimum)));
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(physical_maximum)));
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(digital_minimum)));
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(digital_maximum)));
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(pre_filtering)));
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(nr_of_samples_in_signal)));
		TRANSMISSION_ERROR(SendHeadersAttribute(TARGET_BDF_MEMBER(reserved)));

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
			return 0;
		}
		return std::strtol(reqRecordsBuffer + file::BDF_COMMANDS::REQ_RECORDS.size() + 1, nullptr, 10);
	}

	void TelemetryTransmitter::BeginTransmission(long const& numberOfMeasurements)
	{
		// Send records
		unsigned writtenBytes = 0;
		PRINTI(TELEMETRY_TAG, "Sending %ld data records.\n", numberOfMeasurements);
		for(int measurement = 0; measurement < numberOfMeasurements; measurement++)
		{
			const size_type recordWrittenBytes = SendDataRecord();
			if(recordWrittenBytes == 0)
			{
				PRINTI(TELEMETRY_TAG, "Failed to send data record.\n");
				break;
			}
			writtenBytes += recordWrittenBytes;
		}
		PRINTI(TELEMETRY_TAG, "Written %u bytes of data records to server\n", writtenBytes);
	}

	void TelemetryTransmitter::BeginTransmission()
	{
		PRINTI(TELEMETRY_TAG, "Sending indefinite\n");
		do
		{
			_socket.SetTimeout(2, 0);
			auto writtenBytes = SendDataRecord();
			if(writtenBytes == 0)
			{
				PRINTI(TELEMETRY_TAG, "Failed to send data record.\n");
				break;
			}
			_socket.SetTimeout(0, 4'000);
		}
		while(!_socket.Check(file::BDF_COMMANDS::REQ_STOP));
	}

	void TelemetryTransmitter::TryAgain()
	{
		// Some error occurred: Could not open socket, Connection lost, etc.
		PRINTI("[Socket:]", "Closing socket and reopening.\n");
		_socket.Close();
		_socket.Open();
	}

	TCPError TelemetryTransmitter::SendHeadersAttribute(size_type const& attributeOffset, size_type const& attributeSize) 
	{
		size_t header = 0;
		for(mem::SensorData<mem::int24_t> const& sensor : _sensorView)
		{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
			for(auto channel = 0; channel < sensor.size(); ++channel)
			{
				const TCPError error = _socket.Send(static_cast<void const*>(&gSignalHeaders[header++]) + attributeOffset, attributeSize);
				if(error != TCPError::NO_ERROR) return error;
			}
#pragma GCC diagnostic pop
		}
		return TCPError::NO_ERROR;
	}

	TelemetryTransmitter::size_type IRAM_ATTR TelemetryTransmitter::SendDataRecord() 
	{
		mem::Stack::size_type channel;
		do
		{
			channel = 0;
			for(mem::SensorData<mem::int24_t>& sensor : _sensorView)
			{
				if(_sendStack.Fits(channel, sizeof(mem::int24_t)) && sensor.ready())
				{
					_sendStack.PushNChannels(sensor.data(), sizeof(mem::int24_t), channel, sensor.size());
				}
				channel += sensor.size();
			}
			YIELD_FOR(4);
		}
		while (!_sendStack.Full(0, channel));
		TRANSMISSION_ERROR(_socket.Send(_sendStack.Data(), _stackSize));
		_sendStack.Clear();
		return _stackSize;
	}
}

