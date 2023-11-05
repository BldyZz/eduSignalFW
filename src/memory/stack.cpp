#include "stack.h"

#include <cstring>

namespace mem
{
	Stack::Stack()
		: _data(nullptr),
		  _size(0),
		  _off(0)
	{
	}

	bool Stack::Fits(util::size_t size) const
	{
		return (_size - _off) >= size;
	}

	void Stack::Push(void* data, util::size_t size) const
	{
		memcpy(_data + _off, data, size);
	}

	void* Stack::Data() const
	{
		return _data;
	}

	util::size_t Stack::Size() const
	{
		return _off;
	}
}
