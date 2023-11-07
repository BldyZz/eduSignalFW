#include "bdf_plus.h"

#include "../util/utils.h"
#include "../util/time.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

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

	//	template<size_t StrSize>
	//	constexpr void string_copy_and_fill(ascii_t* dst, size_t size, std::array<ascii_t, StrSize> arr, char fill_value = ' ')
	//	{
	//		size_t i;
	//		for(i = 0; i < StrSize; i++)
	//		{
	//			dst[i] = arr[i];
	//		}
	//	
	//		for(; i < size; i++)
	//		{
	//			dst[i] = fill_value;
	//		}
	//	}
	//	
	//	constexpr void string_copy_and_fill_s(ascii_t* dst, size_t size, std::string const& str, char fill_value = ' ')
	//	{
	//		size_t i;
	//		for(i = 0; i < str.size(); i++)
	//		{
	//			dst[i] = str[i];
	//		}
	//	
	//		for(; i < size; i++)
	//		{
	//			dst[i] = fill_value;
	//		}
	//	}

	constexpr void string_copy_and_fill(ascii_t* dst, size_t size, const ascii_t* str, char fill_value = ' ')
	{
		size_t i;
		for(i = 0; str[i++] != '\0';)
		{
			dst[i] = str[i];
		}
		for(; i < size; i++)
		{
			dst[i] = fill_value;
		}
	}

	constexpr void string_copy_and_fill(ascii_t* dst, size_t size, ascii_t* str, char fill_value = ' ')
	{
		size_t i;
		for(i = 0; str[i++] != '\0';)
		{
			dst[i] = str[i];
		}
		for(; i < size; i++)
		{
			dst[i] = fill_value;
		}
	}

	void string_copy_and_fill(ascii_t* dst, size_t size, float value, char fill_value = ' ')
	{
		std::stringstream ss;
		ss << std::fixed << std::setprecision(size) << value << '\0';
		string_copy_and_fill(dst, size, ss.str().data());
	}

	void string_copy_and_fill(ascii_t* dst, size_t size, uint32_t value, char fill_value = ' ')
	{
		std::stringstream ss;
		ss << std::setw(size) << value << '\0';
		string_copy_and_fill(dst, size, ss.str().data());
	}

	void string_copy_and_fill(ascii_t* dst, size_t size, int32_t value, char fill_value = ' ')
	{
		std::stringstream ss;
		ss << std::setw(size) << value << '\0';
		string_copy_and_fill(dst, size, ss.str().data());
	}

	void create_general_header(bdf_header_t* header, float duration_of_a_data_record, uint32_t number_of_channels_N_in_data_record)
	{
		string_copy_and_fill(header->version, std::size(header->version), "0");
		string_copy_and_fill(header->local_patient_identification, std::size(header->local_patient_identification), "Test Subject");
		string_copy_and_fill(header->local_recording_identification, std::size(header->local_recording_identification), "Local Record Info");

		string_copy_and_fill(header->number_of_bytes_in_header_record, std::size(header->number_of_bytes_in_header_record), util::total_size<bdf_header_t>());
		string_copy_and_fill(header->version_of_dataformat, std::size(header->version_of_dataformat), "24BIT");
		string_copy_and_fill(header->number_of_data_records, std::size(header->number_of_data_records), "-1"); // (-1 if unknown)

		string_copy_and_fill(header->duration_of_a_data_record, std::size(header->duration_of_a_data_record), duration_of_a_data_record); // in seconds
		string_copy_and_fill(header->number_of_signal_headers, std::size(header->number_of_signal_headers), number_of_channels_N_in_data_record);
		
		// Insert Time
		const time_t current_time = time(nullptr);
		tm time_result = {};
		DISCARD localtime_r(&current_time, OUT &time_result);
		std::array<ascii_t, 8> date;
		(void)std::strftime(date.data(), std::size(date), "%d/%m/%y", &time_result);
		string_copy_and_fill(header->startdate_of_recording, std::size(header->startdate_of_recording), date.data()); // (dd.mm.yy)
		(void)std::strftime(date.data(), std::size(date), "%H/%M/%S", &time_result);
		string_copy_and_fill(header->starttime_of_recording, std::size(header->starttime_of_recording), date.data()); // (hh.mm.ss)
	}

	void create_record_header(OUT bdf_record_header_t* header,
							  const ascii_t* label,
							  const ascii_t* transducer_type,
							  const ascii_t* physical_dimension,
							  int32_t physical_minimum,
							  int32_t physical_maximum,
							  int32_t digital_minimum,
							  int32_t digital_maximum,
							  const ascii_t* pre_filtering,
							  uint32_t nr_of_samples_in_data_record)
	{
		string_copy_and_fill(header->label, std::size(header->label), label);
		string_copy_and_fill(header->transducer_type, std::size(header->transducer_type), transducer_type);
		string_copy_and_fill(header->physical_dimension, std::size(header->physical_dimension), physical_dimension);
		string_copy_and_fill(header->physical_minimum, std::size(header->physical_minimum), physical_minimum);
		string_copy_and_fill(header->physical_maximum, std::size(header->physical_maximum), physical_maximum);
		string_copy_and_fill(header->digital_minimum, std::size(header->digital_minimum), digital_minimum);
		string_copy_and_fill(header->digital_maximum, std::size(header->digital_maximum), digital_maximum);
		string_copy_and_fill(header->pre_filtering, std::size(header->pre_filtering), pre_filtering);
		string_copy_and_fill(header->nr_of_samples_in_data_record, std::size(header->nr_of_samples_in_data_record), nr_of_samples_in_data_record);
		string_copy_and_fill(header->reserved, std::size(header->reserved), "");

		//string_copy_and_fill(header->label, std::size(header->label), non_terminated())
	}
}
	