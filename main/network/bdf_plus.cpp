#include "bdf_plus.h"

#include <cassert>

#include "../util/utils.h"
#include "../util/time.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace file
{
	constexpr void string_copy_and_fill(ascii_t* dst, size_t size, const ascii_t* str)
	{
		size_t i;
		for(i = 0; str[i] != '\0' && i < size; i++)
		{
			dst[i] = str[i];
		}
		for(; i < size; i++)
		{
			dst[i] = ' ';
		}
	}

	constexpr void string_copy_and_fill(ascii_t* dst, size_t size, ascii_t* str)
	{
		size_t i;
		for(i = 0; str[i] != '\0' && i < size; i++)
		{
			dst[i] = str[i];
		}
		for(; i < size; i++)
		{
			dst[i] = ' ';
		}
	}

	void string_copy_and_fill(ascii_t* dst, size_t size, float value)
	{
		std::stringstream ss;
		ss << std::fixed << std::setprecision(size) << value << '\0';
		string_copy_and_fill(dst, size, ss.str().data());
	}

	void string_copy_and_fill(ascii_t* dst, size_t size, uint32_t value)
	{
		std::stringstream ss;
		ss << std::left << value << '\0';
		string_copy_and_fill(dst, size, ss.str().data());
	}

	void string_copy_and_fill(ascii_t* dst, size_t size, int32_t value)
	{
		std::stringstream ss;
		ss << std::left << value << '\0';
		string_copy_and_fill(dst, size, ss.str().data());
	}

#define TARGET_BDF_HEADER_MEMBER(header_ptr, member) header_ptr->member, sizeof std::remove_pointer_t<decltype(header_ptr)>::member

	void create_general_header(bdf_header_t* header, float duration_of_a_data_record, int32_t number_of_data_records, uint32_t number_of_channels_N_in_data_record)
	{
		const char* testSubject         = ""; // "Test Subject";
		const char* localRecordInfo     = ""; //"Local Record Info";
		const char* versionOfDataFormat = "24BIT";
		header->version[0] = 255; // 255
		header->version[1] = 'B';
		header->version[2] = 'I';
		header->version[3] = 'O';
		header->version[4] = 'S';
		header->version[5] = 'E';
		header->version[6] = 'M';
		header->version[7] = 'I';

		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, local_patient_identification), testSubject);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, local_recording_identification), localRecordInfo);

		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, number_of_bytes_in_header_record), (1 + number_of_channels_N_in_data_record) * util::total_size<bdf_header_t>());
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, version_of_dataformat), versionOfDataFormat);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, number_of_data_records), number_of_data_records); // (-1 if unknown)

		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, duration_of_a_data_record), duration_of_a_data_record); // in seconds
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, number_of_signal_headers), number_of_channels_N_in_data_record);

		// Get time
		time_t now;
		time(&now);
		setenv("TZ", "CT", 1);
		tzset();
		// Insert Time
		tm time_result = {};
		DISCARD localtime_r(&now, &time_result);
		std::array<ascii_t, 9> date;
		(void)std::strftime(date.data(), date.size(), "%d.%m.%y", &time_result);
		string_copy_and_fill(header->startdate_of_recording, std::size(header->startdate_of_recording), date.data()); // (dd.mm.yy)
		(void)std::strftime(date.data(), date.size(), "%H.%M.%S", &time_result);
		string_copy_and_fill(header->starttime_of_recording, std::size(header->starttime_of_recording), date.data()); // (hh.mm.ss)

		// Assert that ascii symbols are either visible or space
		assert(
			[&](bool isInvalid = false) 
			{
				for(uint32_t index = 1; index < sizeof header->data; index++)
					isInvalid |= header->data[index] < ' ' || header->data[index] > '~';
				return !isInvalid;
			}()
		);
	}

	void create_signal_header(OUT bdf_signal_header_t* header,
							  const ascii_t* label,
							  const ascii_t* transducer_type,
							  const ascii_t* physical_dimension,
							  int32_t physical_minimum,
							  int32_t physical_maximum,
							  int32_t digital_minimum,
							  int32_t digital_maximum,
							  const ascii_t* pre_filtering,
							  uint32_t nr_of_samples_in_signal
							  )
	{
		const char* reserved = "Reserved";

		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, label), label);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, transducer_type), transducer_type);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, physical_dimension), physical_dimension);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, physical_minimum), physical_minimum);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, physical_maximum), physical_maximum);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, digital_minimum), digital_minimum);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, digital_maximum), digital_maximum);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, pre_filtering), pre_filtering);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, nr_of_samples_in_signal), nr_of_samples_in_signal);
		string_copy_and_fill(TARGET_BDF_HEADER_MEMBER(header, reserved), reserved);

		// Assert that ascii symbols are either visible or space
		assert(
			[&](bool isInvalid = false) 
			{
				for(char const& symbol : header->data)
					isInvalid |= symbol < ' ' || symbol > '~';
				return !isInvalid;
			}()
		);
		//string_copy_and_fill(header->label, std::size(header->label), non_terminated())
	}
}
	