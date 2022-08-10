//
// Created by patrick on 5/25/22.
//

#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include <cstddef>
#include <cstring>
#include <fmt/format.h>

namespace esp {
template<typename Config>
struct spiHost {
private:
    spi_bus_config_t  busConfig;
    spi_host_device_t hostDevice;

public:
    spiHost()
      : busConfig{.mosi_io_num = Config::MOSI, .miso_io_num = Config::MISO, .sclk_io_num = Config::SCK, .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = static_cast<int>(Config::transferSize)}
      , hostDevice{Config::SPIHost} {
        fmt::print("Initializing SPI2...\n");
        ESP_ERROR_CHECK(spi_bus_initialize(hostDevice, &busConfig, Config::DMAChannel));
    }
    spi_host_device_t getHostDevice() const { return hostDevice; }
};

};   // namespace esp
