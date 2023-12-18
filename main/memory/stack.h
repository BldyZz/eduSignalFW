#pragma once

#include <cstddef>

namespace mem
{
	class Stack
	{
	public:
		using size_type = std::size_t;
		using pointer = void*;
		using const_pointer = void const*;

		struct layout_section
		{
			size_type level;
			size_type size;
			size_type off;
		};
		using layout = layout_section*;

	public:
		Stack() = delete;

		template<typename T, size_type Size, size_type ChannelCount>
		explicit Stack(T (&underlyingBuffer)[Size], layout_section (&stackLayout)[ChannelCount])
			: _sdata(&underlyingBuffer[0]),
		      _layout(&stackLayout[0]),
			  _size(Size * sizeof(T)),
			  _channels(ChannelCount)
		{
		}

		void PushNChannels(const_pointer data, size_type const& size, size_type const& firstChannel, size_type const& numberOfChannels) const;
		void Push(const_pointer data, size_type const& size, size_type const& channel) const;

		size_type FreeSpace(size_type const& channel) const;
		void Clear(size_type firstChannel, size_type const& numberOfChannels) const;
		void Clear() const;

		bool Fits(size_type const& channel, size_type const& size) const;
		bool Full(size_type const& channel) const;
		bool Full(size_type const& firstChannel, size_type const& numberOfChannels) const;
		const_pointer  Data() const;

	private:

		using size_array = size_type*;

	private:
		pointer	  _sdata;        // stack data
		layout    _layout; // size of each channel
		size_type _size;         // size of _sdata in bytes
		size_type _channels;
	};
}