#pragma once

#include <cstdint>
#include "../util/defines.h"
#include "../util/types.h"

namespace file
{
	/* First send headers...
	 *---------------------------------------------------------------------*
	 |	bdf_header_t | bdf_record_header_t 1| ... | bdf_record_header_t N  |
	 *---------------------------------------------------------------------*
	 * Then send record data where each data point is 24-Bits
	 * --------------------------------------------------------------------*
	 | data_record 1 |      ...     | data_record N | data_record 1 |  ...      
	 ----------------------------------------------------------------------*
	 */

	/**
	 * \brief General BDF-Header
	 */
	struct bdf_header_t
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

	/**
	 * \brief Header which gets send in the beginning N times after the general header.
	 * This is one of (N) signal headers.
	 */
	struct bdf_record_header_t
	{
		ascii_t label[16];				// (e.g.EEG Fpz - Cz or Body temp) (mind item 9 of the additional EDF + specs)
		ascii_t transducer_type[80];	// (e.g.AgAgCl electrode)
		ascii_t physical_dimension[8];	// (e.g.uV or degreeC)
		ascii_t physical_minimum[8];	// (e.g. - 500 or 34)
		ascii_t physical_maximum[8];	// (e.g. 500 or 40)
		ascii_t digital_minimum[8];		// (e.g. - 2048)
		ascii_t digital_maximum[8];		// (e.g. 2047) 
		ascii_t pre_filtering[80];		// (e.g.HP:0.1Hz LP : 75Hz) 
		ascii_t nr_of_samples_in_data_record[8];
		ascii_t reserved[32];
	};

	struct EP_LABEL
	{
		static constexpr char ECG[]      = "ECG V";
		static constexpr char EEG[]		 = "EEG";
		static constexpr char RED[]		 = "Red";
		static constexpr char INFRARED[] = "Infrared";
	};

	void create_general_header(OUT bdf_header_t* header, float duration_of_a_data_record, uint32_t number_of_channels_N_in_data_record);
	void create_record_header(OUT bdf_record_header_t* header, 
							  const ascii_t* label, 
							  const ascii_t* transducer_type, 
							  const ascii_t* physical_dimension, 
							  int32_t physical_minimum, 
							  int32_t physical_maximum,
							  int32_t digital_minimum,
							  int32_t digital_maximum,
							  const ascii_t* pre_filtering,
							  uint32_t nr_of_samples_in_data_record);
}
