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
    explicit spiHost(
      spi_host_device_t const& spiHostDevice,
      gpio_num_t const        MISO,
      gpio_num_t const        MOSI,
      gpio_num_t const        CLK,
      std::size_t const       transferSize,
      spi_common_dma_t const  dmaEnable) {
        fmt::print("Initializing SPI2...\n");
        std::memset(&busConfig, 0, sizeof(busConfig));
        std::memset(&hostDevice, 0, sizeof(hostDevice));
        busConfig.miso_io_num     = MISO;
        busConfig.mosi_io_num     = MOSI;
        busConfig.sclk_io_num     = CLK;
        busConfig.quadhd_io_num   = -1;
        busConfig.quadwp_io_num   = -1;
        busConfig.max_transfer_sz = transferSize;
        hostDevice                 = spiHostDevice;
        auto ret                  = spi_bus_initialize(hostDevice, &busConfig, dmaEnable);
        ESP_ERROR_CHECK(ret);
    }
    spi_host_device_t getHostDevice() const{
        return hostDevice;
    }
};

};   // namespace esp
