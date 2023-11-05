#pragma once

#include "../util/types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <type_traits>

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
namespace mem
{
	class RingBuffer;
	using RingBufferHandle = RingBuffer*;

	struct RingBufferArray
	{
		size_t 		      size    = 0;
		RingBufferHandle* buffers = nullptr;

		
	};	

	class RingBuffer
	{
	public:
		using size_type = uint16_t;
		using id_type   = uint8_t;

		static constexpr id_type INVALID_ID = -1;
	public:
		RingBuffer();

		template<typename T, util::size_t Size>
		RingBuffer(StaticSemaphore_t* mutexBuffer, T(&buffer)[Size], id_type id, id_type channelCount = 1)
			: _buffer(buffer),
		      _nodeSize(sizeof(T)),
		      _nodeCount(Size / channelCount),
		      _read(0),
			  _write(0),
			  _id(id),
			  _channelCount(channelCount)
		{
			assert(channelCount > 0 && "RingBuffer(...): The channel count of a RingBuffer cannot be 0.");
			_mutex = xSemaphoreCreateMutexStatic(mutexBuffer);
			configASSERT(_mutex);
		}

		template<typename NodeStructure>
		void Write(NodeStructure value)
		{
			xSemaphoreTake(_mutex, portMAX_DELAY);
			static_cast<NodeStructure*>(_buffer)[_write] = value;
			_write = (_write + 1) % _nodeCount;
			xSemaphoreGive(_mutex);
		}
		template<typename NodeStructure>
		NodeStructure Read()
		{
			xSemaphoreTake(_mutex, portMAX_DELAY);
			NodeStructure node = static_cast<NodeStructure*>(_buffer)[_read];
			_read = (_read + 1) % _nodeCount;
			xSemaphoreGive(_mutex);
			return node;
		}

		void  Lock();
		void  Unlock();

		void* ReadAdvance();
		void* WriteAdvance();
		void* ChangeChannel(void* ptr, id_type channelIndex);

		bool	  IsValid() const;
		bool	  HasData() const;

		size_type NodeSize() const;
		size_type Size() const;
		id_type   Id() const;
		id_type   ChannelCount() const;
	private:
		using semphr_t = SemaphoreHandle_t;

		void*	  _buffer;
		semphr_t  _mutex;
		size_type _nodeSize;
		size_type _nodeCount;
		size_type _read;
		size_type _write;
		id_type   _id;
		id_type   _channelCount;
	};
}
