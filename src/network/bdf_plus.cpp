#include "bdf_plus.h"

#include "../util/utils.h"
#include "../util/time.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace file
{
	template<size_t Size>
	constexpr auto non_terminated(const ascii_t(&str)[Size]) -> std::array<ascii_t, Size - 1>
	{
		static_assert(Size > 1, "non_terminated: The size of the given string is 1. The return value can't be of size 0.");
		std::array<ascii_t, Size - 1> ret;
		for(size_t i = 0; i < Size - 1; i++)
		{
			ret[i] = str[i];
		}
		return ret;
	}

	template<size_t StrSize>
	constexpr void string_copy_and_fill(ascii_t* dst, size_t size, std::array<ascii_t, StrSize> arr, char fill_value = ' ')
	{
		size_t i;
		for(i = 0; i < StrSize; i++)
		{
			dst[i] = arr[i];
		}

		for(; i < size; i++)
		{
			dst[i] = fill_value;
		}
	}

	constexpr void string_copy_and_fill_s(ascii_t* dst, size_t size, std::string const& str, char fill_value = ' ')
	{
		size_t i;
		for(i = 0; i < str.size(); i++)
		{
			dst[i] = str[i];
		}

		for(; i < size; i++)
		{
			dst[i] = fill_value;
		}
	}

	void create_general_header(bdf_header_t* header, float duration_of_a_data_record, uint16_t number_of_channels_N_in_data_record)
	{
		std::stringstream ss;
		std::string out;

		string_copy_and_fill(header->version, std::size(header->version), non_terminated("0"));
		string_copy_and_fill(header->local_patient_identification, std::size(header->local_patient_identification), non_terminated("Test Subject"));
		string_copy_and_fill(header->local_recording_identification, std::size(header->local_recording_identification), non_terminated("Local Record Info"));

		ss << std::setprecision(util::total_size(header->number_of_bytes_in_header_record)) << util::total_size<bdf_header_t>();
		out = ss.str();

		string_copy_and_fill_s(header->number_of_bytes_in_header_record, std::size(header->number_of_bytes_in_header_record), out);
		string_copy_and_fill(header->version_of_dataformat, std::size(header->version_of_dataformat), non_terminated("24BIT"));
		string_copy_and_fill(header->number_of_data_records, std::size(header->number_of_data_records), non_terminated("-1")); // (-1 if unknown)

		ss.str("");
		ss << std::fixed << std::setprecision(util::total_size(header->duration_of_a_data_record)) << duration_of_a_data_record;
		out = ss.str();
		string_copy_and_fill_s(header->duration_of_a_data_record, std::size(header->duration_of_a_data_record), out); // in seconds

		ss.str("");
		ss << std::setprecision(util::total_size(header->number_of_signal_headers)) << number_of_channels_N_in_data_record;
		out = ss.str();
		string_copy_and_fill_s(header->number_of_signal_headers, std::size(header->number_of_signal_headers), out);

		// Insert Time
		const time_t current_time = time(nullptr);
		tm time_result = {};
		DISCARD localtime_r(&current_time, OUT &time_result);
		std::array<ascii_t, 8> date;
		(void)std::strftime(date.data(), std::size(date), "%d/%m/%y", &time_result);
		string_copy_and_fill(header->startdate_of_recording, std::size(header->startdate_of_recording), date); // (dd.mm.yy)
		(void)std::strftime(date.data(), std::size(date), "%H/%M/%S", &time_result);
		string_copy_and_fill(header->starttime_of_recording, std::size(header->starttime_of_recording), date); // (hh.mm.ss)
	}
}
	