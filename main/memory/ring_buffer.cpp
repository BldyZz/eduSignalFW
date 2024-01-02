#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "ring_buffer.h"

#include <cstdio>

namespace mem
{
	RingBufferView::RingBufferView()
	{
		_first = nullptr;
		_size  = 0;
	}

	RingBufferView::RingBufferView(pointer buffer, size_type size)
		: _first(buffer), _size(size)
	{
	}

	void RingBufferView::ResetAll()
	{
		for(mem::RingBuffer* b : *this)
			b->Lock();

		for(mem::RingBuffer* b : *this)
			b->Reset();

		for(mem::RingBuffer* b : *this)
			b->Unlock();
	}

	RingBuffer& RingBufferView::operator[](size_type index) const
	{
		return *_first[index];
	}

	RingBufferView::iterator RingBufferView::begin() const
	{
		return _first;
	}

	RingBufferView::iterator RingBufferView::end() const
	{
		return _first + _size;
	}

	RingBufferView::size_type RingBufferView::size() const
	{
		return _size;
	}

	RingBuffer::RingBuffer()
		: _buffer(nullptr),
		_headers(nullptr),
		_mutex(nullptr),
		_nodeSize(0),
		_nodeCount(0),
		_read(0),
		_write(0),
		_channelCount(0)
	{
	}

	RingBuffer::RingBuffer(StaticSemaphore_t* mutexBuffer, 
						   void* underlyingBuffer, 
						   size_type nodeSize,
						   size_type nodeCount, 
						   channel_t channelCount)
							   : _buffer(underlyingBuffer),
								 _headers(nullptr),
								 _nodeSize(nodeSize),
								 _nodeCount(nodeCount),
								 _read(0),
								 _write(0),
							 	 _nodesInBDFRecord(0),
								 _channelCount(channelCount)
	{
		bool isPower2 = (_nodeCount & (_nodeCount - 1)) == 0 && _nodeCount;
		if(!isPower2)
		{
			printf("Nodecount: %u\n", _nodeCount);
			assert(isPower2 && "RingBuffer(...): Node Count / Channel is not power of 2.");
		}
		
		assert(channelCount > 0 && "RingBuffer(...): The channel count of a RingBuffer cannot be 0.");
		_mutex = xSemaphoreCreateMutexStatic(mutexBuffer);
		configASSERT(_mutex);
	}

	void RingBuffer::Lock()
	{
		xSemaphoreTake(_mutex, portMAX_DELAY);
	}

	void RingBuffer::ReadAdvance(size_type advanceNNodes) noexcept
	{
		_read = (_read + advanceNNodes) % _nodeCount;
	}

	void IRAM_ATTR RingBuffer::WriteAdvance() noexcept
	{
		_write = (_write + 1) % _nodeCount;
	}

	void* RingBuffer::CurrentWrite() const noexcept
	{
		return static_cast<char*>(_buffer) + _write * _nodeSize;
	}

	void* RingBuffer::ChangeChannel(void* ptr, channel_t channelIndex) const noexcept
	{
		return ptr + channelIndex * _nodeCount * _nodeSize;
	}

	void RingBuffer::Unlock() const
	{
		xSemaphoreGive(_mutex);
	}

	bool RingBuffer::CanWrite() const
	{
		return (_write + 1) % _nodeCount != _read;
	}

	void* RingBuffer::CurrentRead() const noexcept
	{
		return static_cast<char*>(_buffer) + _read * _nodeSize;
	}

	bool RingBuffer::IsValid() const
	{
		return _channelCount;
	}

	bool RingBuffer::IsOverflowing() const
	{
		return _write < _read;
	}

	RingBuffer::size_type RingBuffer::NodeSize() const
	{
		return _nodeSize;
	}

	RingBuffer::size_type RingBuffer::Size() const
	{
		return (_write - _read) & (_nodeCount - 1);
	}

	RingBuffer::size_type RingBuffer::NodesToOverflow() const
	{
		return (_nodeCount - _read);
	}

	RingBuffer::channel_t RingBuffer::ChannelCount() const
	{
		return _channelCount;
	}

	void RingBuffer::SetBDF(file::bdf_signal_header_t* headers, size_type const& nodesInBDFRecord)
	{
		_headers = headers;
		_nodesInBDFRecord = nodesInBDFRecord;
	}

	file::bdf_signal_header_t const* RingBuffer::RecordHeaders() const
	{
		return _headers;
	}

	RingBuffer::size_type RingBuffer::NodesInBDFRecord() const
	{
		return _nodesInBDFRecord;
	}

	void RingBuffer::Reset()
	{
		_write = 0;
		_read  = 0;
	}
}
