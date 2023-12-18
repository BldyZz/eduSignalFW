#pragma once

#include <future>
#include <span>
#include "../network/bdf_plus.h"

namespace mem
{
	template<typename T>
	class BDFFuture : public std::shared_future<std::span<T>>
	{
	public:
		using value_type = T;
		using size_type = uint32_t;
		using base_type = std::shared_future<std::span<value_type>>;
		using future_type = std::future<std::span<value_type>>;
		using my_type   = BDFFuture;

		BDFFuture() = delete;
		explicit BDFFuture(file::bdf_signal_header_t* signalHeaders, size_type nodesInBDFRecord, size_type channels)
		: base_type(),
			_signalHeaders(signalHeaders),
			_nodesInBDFRecord(nodesInBDFRecord),
			_channels(channels)
		{
		}

		BDFFuture(my_type const& other) noexcept
			: base_type(other),
			_signalHeaders(other._signalHeaders),
			_nodesInBDFRecord(other._nodesInBDFRecord),
			_channels(other._channels)
		{
		}

		my_type& operator =(const my_type& other)
		{
			_signalHeaders      = other._signalHeaders;
			_nodesInBDFRecord   = other._nodesInBDFRecord;
			_channels           = other._channels;
			base_type::operator =(other);
			return *this;
		}



		BDFFuture(my_type&& other) noexcept
		: base_type(std::move(other)),
			_signalHeaders(other._signalHeaders),
		    _nodesInBDFRecord(other._nodesInBDFRecord),
			_channels(other._channels)
		{
		}

		explicit BDFFuture(base_type&& future, file::bdf_signal_header_t* signalHeaders, size_type const& nodesInBDFRecord, size_type const& channels) noexcept
		: base_type(std::forward<base_type>(future)),
			_signalHeaders(signalHeaders),
			_nodesInBDFRecord(nodesInBDFRecord),
			_channels(channels)
		{
		}

		my_type& operator =(my_type&& other) noexcept
		{
			_nodesInBDFRecord   = other._nodesInBDFRecord;
			_signalHeaders      = other._signalHeaders;
			_channels           = other._channels;
			base_type::operator =(std::move(other));
			return *this;
		}

		size_type                        ChannelCount()     const { return _channels; }
		size_type				         NodesInBDFRecord() const { return _nodesInBDFRecord; }
		file::bdf_signal_header_t const* BDFSignalHeaders() const { return _signalHeaders; }
	private:
		file::bdf_signal_header_t* _signalHeaders;
		size_type _nodesInBDFRecord;
		size_type _channels;
	};
}
