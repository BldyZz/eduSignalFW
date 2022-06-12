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
struct spiHost {
private:
    spi_bus_config_t  busConfig;
    spi_host_device_t hostDevice;

public:
    spiHost(
      spi_host_device_t const spiHostDevice,
      gpio_num_t const         MISO,
      gpio_num_t const         MOSI,
      gpio_num_t const         CLK,
      std::size_t const        transferSize,
      spi_common_dma_t const   dmaEnable)
      : busConfig{ .mosi_io_num = MOSI, .miso_io_num = MISO,
                  .sclk_io_num = CLK, .quadwp_io_num = -1,
                  .quadhd_io_num = -1, .max_transfer_sz = static_cast<int>(transferSize)}
       , hostDevice{spiHostDevice} {
        fmt::print("Initializing SPI2...\n");
        ESP_ERROR_CHECK(spi_bus_initialize(hostDevice, &busConfig, dmaEnable));
    }
    spi_host_device_t getHostDevice() const { return hostDevice; }
};

};   // namespace esp
