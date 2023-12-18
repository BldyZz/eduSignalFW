#pragma once

#include "../memory/ring_buffer.h"
#include "../memory/stack.h"
#include "tcp_client.h"
#include "esp_attr.h"

namespace mem
{
	struct int24_t;
	struct RingBufferView;
}

namespace net
{
	class TelemetryTransmitter
	{
	public:
		TelemetryTransmitter() = delete;
		TelemetryTransmitter(mem::RingBufferView const* view);

		bool FindServer();
		bool SendHeaders();
		long GetNumberOfMeasurements();
		void BeginTransmission(long const& numberOfMeasurements);
		void BeginTransmission(); // Sends indefinite until BDF Command is received.

	private:
		using size_type = size_t;

		void SendHeadersAttribute(size_type const& attributeOffset, size_type const& attributeSize);
		size_type SendDataRecord();
		size_type IRAM_ATTR SendDataRecord2();

		mem::RingBufferView _bufferView;
		mem::Stack     _sendStack;
		net::TCPClient _socket;
		unsigned       _channelCount;
		size_type      _stackSize;
	};
}


