#include "ring_buffer.h"

namespace mem
{
	RingBuffer::RingBuffer()
		: _buffer(nullptr),
		_mutex(nullptr),
		_nodeSize(0),
		_nodeCount(0),
		_read(0),
		_write(0),
		_id(RingBuffer::INVALID_ID),
		_channelCount(0)
	{
	}

	void RingBuffer::Lock()
	{
		xSemaphoreTake(_mutex, portMAX_DELAY);
	}

	void* RingBuffer::ReadAdvance()
	{
		auto advance_ptr =  static_cast<char*>(_buffer) + _read * _nodeSize;
		_read = (_read + 1) % _nodeCount;
		return advance_ptr;
	}

	void* RingBuffer::WriteAdvance()
	{
		auto advance_ptr = static_cast<char*>(_buffer) + _write * _nodeSize;
		_write = (_write + 1) % _nodeCount;
		return advance_ptr;
	}

	void* RingBuffer::ChangeChannel(void* ptr, id_type channelIndex)
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

	RingBuffer::id_type RingBuffer::Id() const
	{
		return _id;
	}

	RingBuffer::id_type RingBuffer::ChannelCount() const
	{
		return _id;
	}
}
