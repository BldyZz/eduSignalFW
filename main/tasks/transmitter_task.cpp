#include "transmitter_task.h"

#include "task_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../util/time.h"

#include "../network/wifi.hpp"
#include "../network/tcp_client.h"
#include "../network/telemetry_transmitter.h"
#include "../memory/nvs.h"


#define TELEMETRY_TAG "[TelemetryTask:]"

namespace mem
{
	struct int24_t;
}

/**
 * \brief Definitions
 */
namespace sys
{
	void transmitter_task(void*)
	{
		esp_util::nvs_init();
		// Establish wifi connection
		const char* ssid = "WLAN-Q3Q83P_EXT";
		const char* pw = "1115344978197496";
		net::connect(ssid, pw);
		net::wait_for_connection();
		net::print_ip_info();
		net::TelemetryTransmitter telemetry;
		while (true)
		{
			if(!telemetry.FindServer()) continue;
			if(!telemetry.SendHeaders()) continue;
			const long numberOfMeasurements = telemetry.GetNumberOfMeasurements();
			if(numberOfMeasurements > 0)
			{
				telemetry.BeginTransmission(numberOfMeasurements);
			}
			else if(numberOfMeasurements < 0)
			{
				telemetry.BeginTransmission();
			}
			// else continue
			YIELD_FOR(5'000);
		}
	}
}
