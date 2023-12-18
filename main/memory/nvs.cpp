#include "nvs_flash.h"

#include <cstdio>

#include "../util/defines.h"
#include "nvs.h"

namespace esp_util
{
    void nvs_init()
    {
        /* Note that if nvs_flash_init() is not returning but crashing, then the partitioning is not setup correctly! */
        // Initialize NVS
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK( err );
	    PRINTI("[NVS:]", "NVS Flash initialized.\n");
    }
}

