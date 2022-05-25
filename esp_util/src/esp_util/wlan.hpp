#pragma once

#include "esp_util/nvs.hpp"

#include <condition_variable>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <mutex>
#include <string>

namespace esp {
struct Wlan {
private:
    std::string hostname;

    esp_netif_t* netif     = nullptr;
    bool         connected = false;

    bool                         eventsRegistered = false;
    esp_event_handler_instance_t instance_wifi;
    esp_event_handler_instance_t instance_ip;

    std::condition_variable connect_cv;
    std::mutex              m;
    void                    registerEvents_() {
        if(eventsRegistered) {
            return;
        }
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
          WIFI_EVENT,
          ESP_EVENT_ANY_ID,
          &event_handler_,
          this,
          &instance_wifi));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
          IP_EVENT,
          ESP_EVENT_ANY_ID,
          &event_handler_,
          this,
          &instance_ip));
        eventsRegistered = true;
    }
    void deregisterEvents_() {
        if(!eventsRegistered) {
            return;
        }

        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_wifi);
        esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, instance_ip);
        eventsRegistered = false;
    }

    static void
    event_handler_(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
        Wlan& wlan = *reinterpret_cast<Wlan*>(arg);
        if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
            esp_wifi_connect();
            std::lock_guard<std::mutex> l(wlan.m);
            wlan.connected = false;
        } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            bool notify = false;
            {
                std::lock_guard<std::mutex> l(wlan.m);
                if(wlan.netif) {
                    wlan.connected = true;
                    notify         = true;
                }
            }
            if(notify) {
                wlan.connect_cv.notify_all();
            }
        }
    }

public:
    explicit Wlan(std::string hostname_, bool wait_for_connect_, bool dummy = false)
      : hostname{hostname_} {
        if(dummy) {
            connected = true;
            return;
        }
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        if(wait_for_connect_) {
            while(true) {
                ESP_LOGW("CONNECT", "wlan connecting");
                connect();
                if(wait_for_connect(std::chrono::seconds{10})) {
                    ESP_LOGW("CONNECT", "wlan connected");
                } else {
                    ESP_LOGW("CONNECT", "wlan connect timeout");
                }
                return;
            }
        }
    }

    ~Wlan() {
        disconnect();
        deregisterEvents_();
        //TODO
    }

    bool has_support() const { return true; }

    bool isConnected() {
        std::lock_guard<std::mutex> l(m);
        return connected;
    }

    void setDefaultStation(std::pair<std::string, std::string> const& station) {
        std::lock_guard<std::mutex> l(m);
        setNVSValue("ssid", station.first);
        setNVSValue("pw", station.second);
    }

    template<typename Rep, typename Per>
    bool wait_for_connect(std::chrono::duration<Rep, Per> timeout) {
        std::unique_lock<std::mutex> l(m);
        return connect_cv.wait_for(l, timeout, [&]() { return connected; });
    }

    void disconnect() {
        std::lock_guard<std::mutex> l(m);
        connected = false;
        deregisterEvents_();
        if(netif) {
            ESP_ERROR_CHECK(esp_wifi_disconnect());
        }
    }
    void connect() { connect(getDefaultStation()); }
    void connect(std::pair<std::string, std::string> const& station) {
        disconnect();

        auto const& ssid = station.first;
        auto const& pw   = station.second;
        if(netif == nullptr) {
            std::lock_guard<std::mutex> l(m);
            netif = esp_netif_create_default_wifi_sta();
            ESP_ERROR_CHECK(esp_netif_set_hostname(netif, hostname.c_str()));
        }

        wifi_sta_config_t station_config{};
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
            registerEvents_();
        }
        ESP_ERROR_CHECK(esp_wifi_start());
        esp_wifi_connect();
    }

    std::pair<std::string, std::string> getDefaultStation() {
        std::lock_guard<std::mutex> l(m);
        return {getNVSValue("ssid", std::string{}), getNVSValue("pw", std::string{})};
    }
};

};   // namespace esp
