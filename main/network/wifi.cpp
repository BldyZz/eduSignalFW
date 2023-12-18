#include "wifi.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include "esp_eap_client.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_netif.h>

#include "../util/defines.h"

#define WLAN_TAG "[WLAN:]"

static constexpr uint32_t IS_CONNECTED_SIGNAL = BIT0;

namespace net
{
	static EventGroupHandle_t wifi_event_group;
	static esp_netif_t* sta_netif = nullptr; // Network station binding

	static void event_callback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
	{
		if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
		{
			esp_wifi_connect();
		}
		else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
		{
			PRINTI(WLAN_TAG, "Disconnected.\n");
			esp_wifi_connect();
			xEventGroupClearBits(wifi_event_group, IS_CONNECTED_SIGNAL);
		}
		else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
		{
			PRINTI(WLAN_TAG, "Got IP. Connection established.\n");
			xEventGroupSetBits(wifi_event_group, IS_CONNECTED_SIGNAL);
		}
	}

	void wifi_init_phase()
	{
		ESP_ERROR_CHECK(esp_netif_init());
		wifi_event_group = xEventGroupCreate();
		ESP_ERROR_CHECK(esp_event_loop_create_default());
		sta_netif = esp_netif_create_default_wifi_sta(); // Create network interface binding
		assert(sta_netif);
		

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // Create and init wifi driver task
		PRINTI(WLAN_TAG, "WIFI configured.\n");


		// Register events
		ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_callback,  nullptr));
		ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_callback, nullptr));
	}

	void wifi_configure_phase(std::string_view ssid, std::string_view pw)
	{
		ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); // Setup RAM as WiFi storage.
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // Setup wifi for STA. This is mandatory in order to enable the later TCP-Connection

		wifi_config_t wifi_config = {};

		assert(sizeof(wifi_config.sta.ssid) >= ssid.size());
		assert(sizeof(wifi_config.sta.password) >= pw.size());
		std::memcpy(wifi_config.sta.ssid, ssid.data(), ssid.size());
		std::memcpy(wifi_config.sta.password, pw.data(), pw.size());

		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	}

	void wifi_start_phase()
	{
		ESP_ERROR_CHECK(esp_wifi_start());
	}

	void print_ip_info()
	{
		esp_netif_ip_info_t ipInfo;
		esp_netif_get_ip_info(sta_netif, &ipInfo);

		PRINTI(WLAN_TAG, "ip = '" IPSTR "'\n", IP2STR(&ipInfo.ip));
	}

	void connect(std::string_view ssid, std::string_view pw)
	{
		net::wifi_init_phase();
		net::wifi_configure_phase(std::forward<decltype(ssid)>(ssid), std::forward<decltype(pw)>(pw));
		net::wifi_start_phase();
	}

	void wait_for_connection()
	{
		DISCARD xEventGroupWaitBits(wifi_event_group, IS_CONNECTED_SIGNAL, pdFALSE, pdFALSE, portMAX_DELAY);
	}

	bool is_connected()
	{
		return xEventGroupGetBits(wifi_event_group) & IS_CONNECTED_SIGNAL;
	}
}
