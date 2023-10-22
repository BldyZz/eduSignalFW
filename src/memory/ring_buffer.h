#pragma once

#include "../util/types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace mem
{
	struct ring_buffer_t
	{
		util::byte*       buffer     = nullptr;
		SemaphoreHandle_t mutex      = nullptr;
		size_t			  node_size  = 0;
		size_t			  node_count = 0;
		size_t			  head       = 0;
		size_t			  tail       = 0;
	};

	inline bool hasData(const ring_buffer_t* ringBuffer)
	{
		return ringBuffer->head != ringBuffer->tail;
	}

	template<typename NodeStructure>
	void write(ring_buffer_t* ringBuffer, NodeStructure value)
	{
		xSemaphoreTake(ringBuffer->mutex, portMAX_DELAY);
		reinterpret_cast<NodeStructure*>(ringBuffer->buffer)[ringBuffer->tail] = value;
		ringBuffer->tail                                                       = (ringBuffer->tail + 1) % ringBuffer->node_count;
		xSemaphoreGive(ringBuffer->mutex);
	}

	template<typename NodeStructure>
	NodeStructure read(ring_buffer_t* ringBuffer)
	{
		xSemaphoreTake(ringBuffer->mutex, portMAX_DELAY);
		NodeStructure node = reinterpret_cast<NodeStructure*>(ringBuffer->buffer)[ringBuffer->head];
		ringBuffer->head = (ringBuffer->head + 1) % ringBuffer->node_count;
		xSemaphoreGive(ringBuffer->mutex);
		return node;
	}

	template<typename NodeStructure, size_t Size>
	ring_buffer_t createStaticRingBuffer(NodeStructure (&buffer)[Size], StaticSemaphore_t* mutexBuffer)
	{
		SemaphoreHandle_t mutex;
		mutex = xSemaphoreCreateMutexStatic(mutexBuffer);
		configASSERT(mutex);

		return ring_buffer_t
		{
			.buffer     = reinterpret_cast<util::byte*>(&buffer[0]),
			.mutex      = mutex,
			.node_size  = sizeof(NodeStructure),
			.node_count = Size,
			.head       = 0,
			.tail       = 0,
		};
	}
}
