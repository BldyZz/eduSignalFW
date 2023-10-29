/*
 *	It is mandatory to follow these steps in this order:
 *	1. With the main task call 'void wifi_init_phase()'.
 *	2. Call 'void wifi_configure_phase()'.
 *	3. Start the WiFi with 'void wifi_start_phase()'.
 *	4. (Optional) When deinit call 'void wifi_disconnect()'.
 *
 */
#pragma once

#include <string_view>

namespace net
{
	void connect(std::string_view ssid, std::string_view pw);
	void wait_for_connection();
	bool is_connected();

	static void wifi_init_phase(); // Initializes Events and WiFi Tasks
	static void wifi_configure_phase(std::string_view ssid, std::string_view pw); // Configures the WiFi
	static void wifi_start_phase();
	void get_ip_info();
}