#pragma once

#include <cstdint>

namespace format
{
	using ascii_t  = char;
	using sample_t = int16_t;

	/**
	 * \brief General 
	 */
	struct edf_plus_general_header_t
	{
		ascii_t version[8];
		ascii_t local_patient_identification[80] = {};
		ascii_t local_recording_identification[80] = {};
		ascii_t startdate_of_recording[8]; // (dd.mm.yy)
		ascii_t starttime_of_recording[8]; // (hh.mm.ss)
		ascii_t number_of_bytes_in_header_record[8];
		ascii_t reserved[44];
		ascii_t number_of_data_records[8]; // (-1 if unknown)
		ascii_t duration_of_a_data_record[8]; // in seconds
		ascii_t number_of_signals_in_data_record[4];
	};

	/**
	 * \brief Header which which gets send on a per data record basis
	 */
	struct edf_plus_signal_header_t
	{
		ascii_t label[16];				// (e.g.EEG Fpz - Cz or Body temp) (mind item 9 of the additional EDF + specs)
		ascii_t transducer_type[80];	// (e.g.AgAgCl electrode)
		ascii_t physical_dimension[8];	// (e.g.uV or degreeC)
		ascii_t physical_minimum[8];	// (e.g. - 500 or 34)
		ascii_t physical_maximum[8];	// (e.g. 500 or 40)
		ascii_t digital_minimum[8];		// (e.g. - 2048)
		ascii_t digital_maximum[8];		// (e.g. 2047) 
		ascii_t prefiltering[80];		// (e.g.HP:0.1Hz LP : 75Hz) 
		ascii_t nr_of_samples_in_each_data_record[8];
		ascii_t reserved[32];
	};

	struct edf_plus_data_record_t
	{
		sample_t samples[];
	};


}
