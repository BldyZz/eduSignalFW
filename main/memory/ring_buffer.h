#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_attr.h"

/** Memory Layout of a Ringbuffer with N channels of equal size.
* Legend: 
*	d*: Data pointer 
*	r*: Read pointer  
*	w*: Write pointer
*	nc: Node count
*	N:  Number of channels
* 
* +-------------------------+-----+-------------------------+
* | Channel 0               | ... | Channel N               |
* +-------------------------+-----+-------------------------+
* |           |           |       |           |           |
* d*          r*         w*  d* + nc * N     d* + nc*N   w* + nc*N
* 
* When r* or w* reach the end of the first channel, it will reset to d*.
* 
**/
namespace file
{
	union bdf_signal_header_t;
}

namespace mem
{
	class RingBuffer;

	struct RingBufferView
	{	
		using value_type     = RingBuffer;
		using handle         = value_type*;
		using pointer        = handle*;
		using iterator       = pointer;
		using const_iterator = const pointer;
		using size_type      = size_t;

		RingBufferView();
		RingBufferView(pointer buffer, size_type size);

		void ResetAll();

		iterator begin() const;
		iterator end() const;

		size_type size() const;

		RingBuffer& operator[](size_type index) const;

	private:
		pointer   _first;
		size_type _size;
	};	

	class RingBuffer
	{
	public:
		using size_type = unsigned int;
		using channel_t = unsigned char;
	public:
		RingBuffer();

		RingBuffer(StaticSemaphore_t* mutexBuffer,
				   void* underlyingBuffer,
				   size_type nodeSize,
				   size_type nodeCount,
				   channel_t channelCount);

		void  Lock();
		void  Unlock() const;

		bool           CanWrite() const;
		void*          ReadAdvance(size_type advanceNNodes) noexcept;
		void IRAM_ATTR WriteAdvance() noexcept;
		void* IRAM_ATTR CurrentWrite() const noexcept;
		void* ChangeChannel(void* ptr, channel_t channelIndex) const noexcept;

		bool IsValid() const;
		bool IsOverflowing() const;
	 	__attribute__((always_inline)) bool HasData() const
	 	{
			return _write != _read;
	 	}

		size_type                        NodeSize() const;
		size_type                        Size() const;
		size_type                        NodesToOverflow() const;
		channel_t                        ChannelCount() const;
		void                             SetBDF(file::bdf_signal_header_t* headers, size_type const& nodesInBDFRecord) ;
		file::bdf_signal_header_t const* RecordHeaders() const;
		size_type                        NodesInBDFRecord() const;
		void                             Reset();

	public:
		void*	  _buffer;
		file::bdf_signal_header_t* _headers;
		SemaphoreHandle_t  _mutex;
		size_type _nodeSize;
		size_type _nodeCount;
		size_type _read;
		size_type _write;
		size_type _nodesInBDFRecord;
		channel_t _channelCount;
	};

}
