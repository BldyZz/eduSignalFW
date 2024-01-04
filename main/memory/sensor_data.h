#pragma once

#include <span>
#include <chrono>

#include "../network/bdf_plus.h"

namespace mem
{
	template<typename T>
	class SensorData : public std::span<T>
	{
	public:
		using base_type = std::span<T>;
		using clock_t = std::chrono::high_resolution_clock;
		using time_point_t = std::chrono::time_point<clock_t>;

	public:
		SensorData(T* begin, std::size_t const& channels, std::size_t const& sampleRate)
			: base_type(begin, channels), lastTime(clock_t::now()), samplePeriod(SensorData::sampleRateToSamplePeriod(sampleRate))
		{
		}

		bool ready()
		{
			const auto now = clock_t::now();
			const bool pastTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() >= samplePeriod;
			if(pastTime)
			{
				lastTime = now;
			}
			return pastTime;
		}

	private:
		static std::size_t sampleRateToSamplePeriod(std::size_t const& sampleRate)
		{
			return 1000.f / static_cast<float>(sampleRate);
		}

		time_point_t lastTime;
		std::size_t samplePeriod; // in ms
	};

	template<typename T>
	class SensorView
	{
	public:
		using element_type = SensorData<T>;
		using value_type = typename element_type::value_type;
		using pointer = element_type*;
		using const_pointer = const element_type*;
		using reference = element_type&;
		using const_reference = const element_type&;
		using iterator = pointer;

	public:
		SensorView()
			: sensorData(nullptr), sensors(0)
		{
		}

		template<typename It>
		SensorView(It begin, It end)
			: sensorData(begin), sensors(std::distance(begin, end))
		{
		}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
		iterator begin() { return sensorData; }
		iterator end() { return sensorData + sensors; }
#pragma GCC diagnostic pop

		size_t size()   const { return sensors; }
		bool   empty()  const { return !sensors; }
		operator bool() const { return sensors; }

	private:
		pointer sensorData;
		std::size_t   sensors;
	};
}