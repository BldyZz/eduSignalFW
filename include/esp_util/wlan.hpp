#pragma once

#include "esp_util/nvs.hpp"

#include <condition_variable>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <mutex>
#include <string>
#include <string_view>

#define DEFAULT_SCAN_LIST_SIZE 16

namespace esp
{
	class Wlan 
	{
	public:
		explicit Wlan(std::string hostname_, bool wait_for_connect_, bool dummy = false);
		~Wlan();

		bool has_support() const;
		bool isConnected();
		std::pair<std::string, std::string> getDefaultStation();

		void setDefaultStation(std::string_view ssid, std::string_view pw);

		template<typename Rep, typename Per>
		bool wait_for_connect(std::chrono::duration<Rep, Per> timeout);

		void connect();
		void connect(std::string_view ssid, std::string_view pw);
		void disconnect();

	private:
		void registerEvents();
		void deregisterEvents();

		static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
	
		std::string                  hostname;
		esp_netif_t*                 netif;
		bool                         connected;
		bool                         eventsRegistered;
		esp_event_handler_instance_t instance_wifi;
		esp_event_handler_instance_t instance_ip;
		std::condition_variable      connect_cv;
		std::mutex                   m;
	};

	template <typename Rep, typename Per>
	bool Wlan::wait_for_connect(std::chrono::duration<Rep, Per> timeout)
	{
		std::unique_lock<std::mutex> l(m);
		return connect_cv.wait_for(l, timeout, [&]() { return connected; });
	}

	void Wlan::registerEvents()
	{
		if(!eventsRegistered)
		{
			ESP_ERROR_CHECK(esp_event_handler_instance_register(
				WIFI_EVENT,
				ESP_EVENT_ANY_ID,
				&event_handler,
				this,
				&instance_wifi));
			ESP_ERROR_CHECK(esp_event_handler_instance_register(
				IP_EVENT,
				ESP_EVENT_ANY_ID,
				&event_handler,
				this,
				&instance_ip));
			eventsRegistered = true;
		}
	}

	void Wlan::deregisterEvents()
	{
		if(eventsRegistered)
		{
			esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_wifi);
			esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, instance_ip);
			eventsRegistered = false;
		}
	}

	void Wlan::event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
	{
		Wlan& wlan = *reinterpret_cast<Wlan*>(arg);

		if(event_base == WIFI_EVENT)
		{
			if(event_id == WIFI_EVENT_STA_DISCONNECTED)
			{
				fmt::print("[WLAN] Disconnected.\n");
				esp_wifi_connect();
				std::lock_guard<std::mutex> l(wlan.m);
				wlan.connected = false;
			}
			else if(event_id == WIFI_EVENT_STA_CONNECTED)
			{
				fmt::print("[WLAN] Connection established.\n");
				std::lock_guard<std::mutex> l(wlan.m);
				wlan.connected = true;
			}
		}
		else if(event_base == IP_EVENT)
		{
			if(event_id == IP_EVENT_STA_GOT_IP)
			{
				fmt::print("[WLAN] Got IP.\n");
				bool notify = false;
				{
					std::lock_guard<std::mutex> l(wlan.m);
					if(wlan.netif)
					{
						wlan.connected = true;
						notify = true;
					}
				}
				if(notify)
				{
					wlan.connect_cv.notify_all();
				}
			}
		}
	}

	Wlan::Wlan(std::string hostname_, bool wait_for_connect_, bool dummy)
		: hostname{hostname_}, netif(nullptr), connected(false), eventsRegistered(false)
	{
		if(dummy)
		{
			connected = true;
			return;
		}
		ESP_ERROR_CHECK(esp_netif_init());
		ESP_ERROR_CHECK(esp_event_loop_create_default());
		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&cfg));
		ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
		ESP_ERROR_CHECK(esp_wifi_start());

		//wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		//ESP_ERROR_CHECK(esp_wifi_init(&cfg));

		if(wait_for_connect_)
		{
			while(true)
			{
				ESP_LOGW("CONNECT", "wlan connecting");
				connect();
				if(wait_for_connect(std::chrono::seconds{10}))
				{
					ESP_LOGW("CONNECT", "wlan connected");
				} else
				{
					ESP_LOGW("CONNECT", "wlan connect timeout");
				}
				return;
			}
		}
	}

	Wlan::~Wlan()
	{
		disconnect();
		deregisterEvents();
		//TODO
	}

	static void print_auth_mode(int authmode)
	{
		switch(authmode)
		{
		case WIFI_AUTH_OPEN:
			fmt::print("Authmode \tWIFI_AUTH_OPEN");
			break;
		case WIFI_AUTH_OWE:
			fmt::print("Authmode \tWIFI_AUTH_OWE");
			break;
		case WIFI_AUTH_WEP:
			fmt::print("Authmode \tWIFI_AUTH_WEP");
			break;
		case WIFI_AUTH_WPA_PSK:
			fmt::print("Authmode \tWIFI_AUTH_WPA_PSK");
			break;
		case WIFI_AUTH_WPA2_PSK:
			fmt::print("Authmode \tWIFI_AUTH_WPA2_PSK");
			break;
		case WIFI_AUTH_WPA_WPA2_PSK:
			fmt::print("Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
			break;
		case WIFI_AUTH_ENTERPRISE:
			fmt::print("Authmode \tWIFI_AUTH_ENTERPRISE");
			break;
		case WIFI_AUTH_WPA3_PSK:
			fmt::print("Authmode \tWIFI_AUTH_WPA3_PSK");
			break;
		case WIFI_AUTH_WPA2_WPA3_PSK:
			fmt::print("Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
			break;
		case WIFI_AUTH_WPA3_ENT_192:
			fmt::print("Authmode \tWIFI_AUTH_WPA3_ENT_192");
			break;
//		case WIFI_AUTH_WPA3_EXT_PSK:
//			fmt::print("Authmode \tWIFI_AUTH_WPA3_EXT_PSK");
//			break;
		default:
			fmt::print("Authmode \tWIFI_AUTH_UNKNOWN");
			break;
		}
	}

	static void print_cipher_type(int pairwise_cipher, int group_cipher)
	{
		switch(pairwise_cipher)
		{
		case WIFI_CIPHER_TYPE_NONE:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
			break;
		case WIFI_CIPHER_TYPE_WEP40:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
			break;
		case WIFI_CIPHER_TYPE_WEP104:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
			break;
		case WIFI_CIPHER_TYPE_TKIP:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
			break;
		case WIFI_CIPHER_TYPE_CCMP:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
			break;
		case WIFI_CIPHER_TYPE_TKIP_CCMP:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
			break;
		case WIFI_CIPHER_TYPE_AES_CMAC128:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
			break;
		case WIFI_CIPHER_TYPE_SMS4:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_SMS4");
			break;
		case WIFI_CIPHER_TYPE_GCMP:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP");
			break;
		case WIFI_CIPHER_TYPE_GCMP256:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP256");
			break;
		default:
			fmt::print("Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
			break;
		}

		switch(group_cipher)
		{
		case WIFI_CIPHER_TYPE_NONE:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_NONE");
			break;
		case WIFI_CIPHER_TYPE_WEP40:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
			break;
		case WIFI_CIPHER_TYPE_WEP104:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
			break;
		case WIFI_CIPHER_TYPE_TKIP:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
			break;
		case WIFI_CIPHER_TYPE_CCMP:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
			break;
		case WIFI_CIPHER_TYPE_TKIP_CCMP:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
			break;
		case WIFI_CIPHER_TYPE_SMS4:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_SMS4");
			break;
		case WIFI_CIPHER_TYPE_GCMP:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_GCMP");
			break;
		case WIFI_CIPHER_TYPE_GCMP256:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_GCMP256");
			break;
		default:
			fmt::print("Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
			break;
		}
	}

	/* Initialize Wi-Fi as sta and set scan method */
	static void wifi_scan(void)
	{
		ESP_ERROR_CHECK(esp_netif_init());
		ESP_ERROR_CHECK(esp_event_loop_create_default());
		esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
		assert(sta_netif);

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&cfg));

		uint16_t number = DEFAULT_SCAN_LIST_SIZE;
		wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
		uint16_t ap_count = 0;
		memset(ap_info, 0, sizeof(ap_info));

		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_start());
		esp_wifi_scan_start(NULL, true);
		fmt::print("Max AP number ap_info can hold = {}", number);
		ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
		ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
		fmt::print("Total APs scanned = {}, actual AP number ap_info holds = {}", ap_count, number);
		for(int i = 0; i < number; i++)
		{
			printf("SSID \t\t%s\n", ap_info[i].ssid);
			printf("RSSI \t\t%d\n", ap_info[i].rssi);
			print_auth_mode(ap_info[i].authmode);
			if(ap_info[i].authmode != WIFI_AUTH_WEP)
			{
				print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
			}
			fmt::print("Channel \t\t{}\n", ap_info[i].primary);
		}

	}

	void check_available_networks()
	{
		// Initialize NVS
		esp_err_t ret = nvs_flash_init();
		if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
		{
			ESP_ERROR_CHECK(nvs_flash_erase());
			ret = nvs_flash_init();
		}
		ESP_ERROR_CHECK(ret);

		wifi_scan();
	}

	bool Wlan::has_support() const
	{
		return true;
	}

	bool Wlan::isConnected()
	{
		std::lock_guard<std::mutex> l(m);
		return connected;
	}

	void Wlan::setDefaultStation(std::string_view ssid, std::string_view pw)
	{
		std::lock_guard<std::mutex> l(m);
		setNVSValue("ssid", ssid.data());
		setNVSValue("pw", pw.data());
	}

	void Wlan::disconnect()
	{
		std::lock_guard<std::mutex> l(m);
		connected = false;
		deregisterEvents();
		if(netif)
		{
			ESP_ERROR_CHECK(esp_wifi_disconnect());
		}
	}

	void Wlan::connect()
	{
		const auto defaultStation = getDefaultStation();
		connect(defaultStation.first, defaultStation.second);
	}

	void Wlan::connect(std::string_view ssid, std::string_view pw)
	{
		disconnect();

		if(netif == nullptr)
		{
			std::lock_guard<std::mutex> l(m);
			netif = esp_netif_create_default_wifi_sta();
			ESP_ERROR_CHECK(esp_netif_set_hostname(netif, hostname.c_str()));
		}

		wifi_sta_config_t station_config;
		assert(sizeof(station_config.ssid) >= ssid.size());
		assert(sizeof(station_config.password) >= pw.size());
		std::memcpy(station_config.ssid, ssid.data(), ssid.size());
		std::memcpy(station_config.password, pw.data(), pw.size());
		wifi_config_t wifi_config{};
		wifi_config.sta = station_config;

		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

		{
			std::lock_guard<std::mutex> l(m);
			registerEvents();
		}
		ESP_ERROR_CHECK(esp_wifi_start());
		esp_wifi_connect();
	}

	std::pair<std::string, std::string> Wlan::getDefaultStation()
	{
		std::lock_guard<std::mutex> l(m);
		return {getNVSValue("ssid", std::string{}), getNVSValue("pw", std::string{})};
	}
};   // namespace esp
