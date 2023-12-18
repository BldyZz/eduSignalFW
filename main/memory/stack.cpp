#include "stack.h"

#include <cassert>
#include <cstring>

namespace mem
{
	void Stack::PushNChannels(const_pointer data, size_type const& size, size_type const& firstChannel,
		size_type const& numberOfChannels) const
	{
		size_type dataOff = 0;
		for(auto channel = firstChannel; channel < (firstChannel + numberOfChannels); ++channel)
		{
			memcpy(_sdata + _layout[channel].off + _layout[channel].level, data + size * dataOff, size);
			_layout[channel].level += size;
			dataOff++;
		}
	}

	void Stack::Push(const_pointer data, size_type const& size, size_type const& channel) const
	{
		memcpy(_sdata + _layout[channel].level, data, size);
		_layout[channel].level = size;
	}

	Stack::size_type Stack::FreeSpace(size_type const& channel) const
	{
		return _layout[channel].size - _layout[channel].level;
	}

	void Stack::Clear(size_type firstChannel, size_type const& numberOfChannels) const
	{
		for(; firstChannel < (firstChannel + numberOfChannels); firstChannel++)
		{
			_layout[firstChannel].level = 0;
		}
	}

	void Stack::Clear() const
	{
		for(size_type channel = 0; channel < _channels; channel++)
		{
			_layout[channel].level = 0;
		}
	}

	bool Stack::Fits(size_type const& channel, size_type const& size) const
	{
		return FreeSpace(channel) >= size;
	}

	bool Stack::Full(size_type const& channel) const
	{
		return _layout[channel].level == _layout[channel].size;
	}

	bool Stack::Full(size_type const& firstChannel, size_type const& numberOfChannels) const
	{
		bool ret = true;
		for(size_type channel = firstChannel; channel < (firstChannel + numberOfChannels); channel++)
			ret &= Full(channel);
		return ret;
	}

	Stack::const_pointer Stack::Data() const
	{
		return _sdata;
	}
}
