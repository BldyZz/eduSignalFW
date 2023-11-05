#pragma once

#include "../util/types.h"

namespace mem
{
	class Stack
	{
	public:
		Stack();
		template<typename T, util::size_t Size>
		Stack(T(&underlyingBuffer)[Size])
		{
			_data = static_cast<util::byte*>(&underlyingBuffer[0]);
			_size = Size * sizeof(T);
			_off  = 0;
		}

		void Push(void* data, util::size_t size) const;
		template<typename T>
		void Push(T& element)
		{
			static_cast<T*>(_data + _off) = element;
			_off += sizeof(element);
		}

		bool		 Fits(util::size_t size) const;
		void*		 Data() const;
		util::size_t Size() const;

	private:
		util::byte*  _data;
		util::size_t _size; // size in bytes
		util::size_t _off; // size of stack data
	};
}