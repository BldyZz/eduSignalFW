//
// Created by patrick on 5/25/22.
//

#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include <cstddef>
#include <cstring>
#include <cstdio>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

namespace esp
{
    template <typename Config>
    struct spiHost
    {
    private:
        spi_bus_config_t busConfig;
        spi_host_device_t hostDevice;

    public:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        spiHost()
            : busConfig{.mosi_io_num = Config::MOSI, .miso_io_num = Config::MISO, .sclk_io_num = Config::SCK, .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = static_cast<int>(Config::transferSize)}, hostDevice{Config::SPIHost}
        {
            ESP_LOGI("[SPI Host:]", "Initializing %s...", Config::Name);
            ESP_ERROR_CHECK(spi_bus_initialize(hostDevice, &busConfig, Config::DMAChannel));
        }
#pragma GCC diagnostic pop

        spi_host_device_t getHostDevice() const
        {
            return hostDevice;
        }
    };

}; // namespace esp
