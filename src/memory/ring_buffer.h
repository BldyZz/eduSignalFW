#pragma once

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
	struct bdf_record_header_t;
}

struct QueueDefinition;

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

		iterator begin();
		iterator end();
		const_iterator begin() const;
		const_iterator end() const;

		size_type size() const;

		RingBuffer& operator[](size_type index);
	private:
		pointer   _first;
		size_type _size;
	};	

	class RingBuffer
	{
	public:
		using size_type = unsigned short int;
		using channel_t = unsigned char;
	public:
		RingBuffer();

		RingBuffer(StaticSemaphore_t* mutexBuffer, 
				   void* underlyingBuffer, 
				   size_type nodeSize, 
				   size_type nodeCount, 
				   channel_t channelCount, 
				   file::bdf_record_header_t* headers, 
				   size_type nodesInBDFRecord);

		//	template<typename NodeStructure>
		//	void Write(NodeStructure value)
		//	{
		//		xSemaphoreTake(_mutex, portMAX_DELAY);
		//		static_cast<NodeStructure*>(_buffer)[_write] = value;
		//		_write = (_write + 1) % _nodeCount;
		//		xSemaphoreGive(_mutex);
		//	}
		//	template<typename NodeStructure>
		//	NodeStructure Read()
		//	{
		//		xSemaphoreTake(_mutex, portMAX_DELAY);
		//		NodeStructure node = static_cast<NodeStructure*>(_buffer)[_read];
		//		_read = (_read + 1) % _nodeCount;
		//		xSemaphoreGive(_mutex);
		//		return node;
		//	}

		void  Lock();
		void  Unlock();

		void* ReadAdvance(size_type advanceNNodes) noexcept;
		void* WriteAdvance() noexcept;
		void* ChangeChannel(void* ptr, channel_t channelIndex) const noexcept;

		bool IsValid() const;
		bool IsOverflowing() const;
		bool HasData() const;

		size_type NodeSize() const;
		size_type Size() const;
		size_type NodesToOverflow() const;
		channel_t ChannelCount() const;
		const file::bdf_record_header_t* RecordHeaders() const;
		size_type NodesInBDFRecord() const;
	private:
		using semphr_t = QueueDefinition*; // = SemaphoreHandle_t

		void*	  _buffer;
		file::bdf_record_header_t* _headers;
		semphr_t  _mutex;
		size_type _nodeSize;
		size_type _nodeCount;
		size_type _read;
		size_type _write;
		size_type _nodesInBDFRecord;
		channel_t _channelCount;
	};

}
