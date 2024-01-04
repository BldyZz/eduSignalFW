#pragma once

#include <cstdint>
#include <cstdio>
#include "../util/defines.h"
#include "../util/types.h"
#include "../util/string_operations.h"


namespace file
{
	/* First send general header, then each attribute of the record gSignalHeaders... 
	 *---------------------------------------------------------------------------------------------------------------------------------------------------------------*
	 |	bdf_header_t | bdf_signal_header_t.label 1| ... | bdf_signal_header_t.label N  | ... | bdf_signal_header_t.reserved 1 | ... | bdf_signal_header_t.reserved N |
	 *---------------------------------------------------------------------------------------------------------------------------------------------------------------*
	 * Then send records where each data point is 24-Bits. A signal contains a predefined number of data points (nr_of_samples_in_signal).
	 * -----------------------------------------------------------------------------------------------------------------------------*
	 | data_record 1: signal 1 |      ...     | data_record 1: signal N | data_record 2: signal 1 |  ... | data_record N: signal N  |   
	 *------------------------------------------------------------------------------------------------------------------------------*
	 */

	/**
	 * \brief General BDF-Header
	 */
	union bdf_header_t
	{
		struct
		{
			ascii_t version[8];
			ascii_t local_patient_identification[80];
			ascii_t local_recording_identification[80];
			ascii_t startdate_of_recording[8]; // (dd.mm.yy)
			ascii_t starttime_of_recording[8]; // (hh.mm.ss)
			ascii_t number_of_bytes_in_header_record[8];
			ascii_t version_of_dataformat[44];
			ascii_t number_of_data_records[8]; // (-1 if unknown)
			ascii_t duration_of_a_data_record[8]; // in seconds
			ascii_t number_of_signal_headers[4]; // = Number of channels (N) in data record
		};
		ascii_t data[256];
	};

	/**
	 * \brief Header which gets send in the beginning N times after the general header.
	 * This is one of (N) signal headers.
	 */
	struct bdf_signal_header_t
	{
		std::size_t samples_in_bdf_record;
		union 
		{
			struct
			{
				ascii_t label[16];				// (e.g.EEG Fpz - Cz or Body temp) (mind item 9 of the additional EDF + specs)
				ascii_t transducer_type[80];	// (e.g.AgAgCl electrode)
				ascii_t physical_dimension[8];	// (e.g.uV or degreeC)
				ascii_t physical_minimum[8];	// (e.g. - 500 or 34)
				ascii_t physical_maximum[8];	// (e.g. 500 or 40)
				ascii_t digital_minimum[8];		// (e.g. - 2048)
				ascii_t digital_maximum[8];		// (e.g. 2047) 
				ascii_t pre_filtering[80];		// (e.g.HP:0.1Hz LP : 75Hz) 
				ascii_t nr_of_samples_in_signal[8];
				ascii_t reserved[32];
			};
			ascii_t data[256];
		};
	};

	struct BDF_COMMANDS
	{
		static constexpr auto DISCOVER           = util::non_terminated("BDF_DISCOVER");   // find firmware
		static constexpr auto ACKNOWLEDGE         = util::non_terminated("BDF_ACK");
		static constexpr auto REQ_HEADER         = util::non_terminated("BDF_REQ_HEADER");
		static constexpr auto REQ_RECORD_HEADERS = util::non_terminated("BDF_REQ_RECORD_HEADERS");
		static constexpr auto REQ_RECORDS        = util::non_terminated("BDF_REQ_RECORDS"); // In seconds (e.g. 0.005). indefinite = 0, until stop command
		static constexpr auto REQ_STOP			= util::non_terminated("BDF_STOP");
	};

	struct EP_LABEL
	{
		static constexpr char ECG[]      = "ECG V";
		static constexpr char EEG[]		 = "EEG";
		static constexpr char RED[]		 = "Red";
		static constexpr char INFRARED[] = "Infrared";
	};

	void create_general_header(OUT bdf_header_t* header, float duration_of_a_data_record, int32_t number_of_data_records, uint32_t number_of_channels_N_in_data_record);
	void create_signal_header(OUT bdf_signal_header_t* header, 
							  const ascii_t* label, 
							  const ascii_t* transducer_type, 
							  const ascii_t* physical_dimension, 
							  int32_t physical_minimum, 
							  int32_t physical_maximum,
							  int32_t digital_minimum,
							  int32_t digital_maximum,
							  const ascii_t* pre_filtering,
							  uint32_t nr_of_samples_in_signal);

	template<typename DeviceType, size_t Count>
	std::size_t createBDFHeader(bdf_signal_header_t(&headers)[Count], std::size_t const& offset)
	{
		for(int header = 0; header < DeviceType::CHANNEL_COUNT; header++)
		{
			//char label[sizeof(DeviceType::LABEL) + 1 + 3 + 1];
			//DISCARD std::snprintf(label, std::size(label), "%s %1d", DeviceType::LABEL, header + 1);
			create_signal_header(&headers[header + offset],
								 DeviceType::LABELS[header],
								 DeviceType::TRANSDUCER_TYPE,
								 DeviceType::PHYSICAL_DIMENSIONS[header],
								 DeviceType::PHYSICAL_MINIMUM,
								 DeviceType::PHYSICAL_MAXIMUM,
								 DeviceType::DIGITAL_MINIMUM,
								 DeviceType::DIGITAL_MAXIMUM,
								 DeviceType::PRE_FILTERING,
								 DeviceType::NODES_IN_BDF_RECORD
			);
			PRINTI("[Header:]", "%s\n", headers[header + offset].data);
		}
		return offset + DeviceType::CHANNEL_COUNT;
	}
}
