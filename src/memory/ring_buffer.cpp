#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "ring_buffer.h"

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

	RingBuffer& RingBufferView::operator[](size_type index)
	{
		return *_first[index];
	}

	RingBufferView::iterator RingBufferView::begin()
	{
		return _first;
	}

	RingBufferView::iterator RingBufferView::end()
	{
		return _first + _size;
	}

	RingBufferView::const_iterator RingBufferView::begin() const
	{
		return _first;
	}

	RingBufferView::const_iterator RingBufferView::end() const
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
						   channel_t channelCount, 
						   file::bdf_record_header_t* headers, 
						   size_type nodesInBDFRecord)
							   : _buffer(underlyingBuffer),
								 _headers(headers),
								 _nodeSize(nodeSize),
								 _nodeCount(nodeCount / channelCount),
								 _read(0),
								 _write(0),
								 _nodesInBDFRecord(nodesInBDFRecord),
								 _channelCount(channelCount)
	{
		assert(channelCount > 0 && "RingBuffer(...): The channel count of a RingBuffer cannot be 0.");
		_mutex = xSemaphoreCreateMutexStatic(mutexBuffer);
		configASSERT(_mutex);
	}

	void RingBuffer::Lock()
	{
		xSemaphoreTake(_mutex, portMAX_DELAY);
	}

	void* RingBuffer::ReadAdvance(size_type advanceNNodes) noexcept
	{
		const auto advance_ptr =  static_cast<char*>(_buffer) + _read * _nodeSize;
		_read = (_read + advanceNNodes) % _nodeCount;
		return advance_ptr;
	}

	void* RingBuffer::WriteAdvance() noexcept
	{
		auto advance_ptr = static_cast<char*>(_buffer) + _write * _nodeSize;
		_write = (_write + 1) % _nodeCount;
		return advance_ptr;
	}

	void* RingBuffer::ChangeChannel(void* ptr, channel_t channelIndex) const noexcept
	{
		return static_cast<char*>(ptr) + channelIndex * _nodeCount * _nodeSize;
	}

	void RingBuffer::Unlock()
	{
		xSemaphoreGive(_mutex);
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
		return _nodeCount;
	}

	bool RingBuffer::HasData() const
	{
		const bool ret = _write != _read;
		return ret;
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

	const file::bdf_record_header_t* RingBuffer::RecordHeaders() const
	{
		return _headers;
	}

	RingBuffer::size_type RingBuffer::NodesInBDFRecord() const
	{
		return _nodesInBDFRecord;
	}
}
