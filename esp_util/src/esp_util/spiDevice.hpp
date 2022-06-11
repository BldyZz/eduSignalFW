//
// Created by patrick on 5/25/22.
//

#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "spiHost.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace esp {
struct spiDevice {
private:
    spi_device_interface_config_t interfaceConfig;
    spi_device_handle_t           spiDeviceHandle;

public:
    explicit spiDevice(
      spiHost const& host,
      std::size_t const   clockSpeed,
      gpio_num_t  const  chipSelect,
      std::size_t const  queueSize,
      std::uint8_t const  spiMode) {
        std::memset(&interfaceConfig, 0, sizeof(interfaceConfig));
        std::memset(&spiDeviceHandle, 0, sizeof(spiDeviceHandle));

        interfaceConfig.clock_speed_hz  = clockSpeed;
        interfaceConfig.mode            = spiMode;
        interfaceConfig.spics_io_num    = chipSelect;
        interfaceConfig.queue_size      = queueSize;
        interfaceConfig.cs_ena_pretrans = 0;
        esp_err_t ret             = spi_bus_add_device(host.getHostDevice(), &interfaceConfig, &spiDeviceHandle);
        ESP_ERROR_CHECK(ret);
    }

    void sendBlocking(std::vector<std::byte> const& data) const{
        spi_transaction_t t;
        std::memset(&t, 0, sizeof(t));
        t.length = data.size()*sizeof(std::byte)*8;
        t.tx_buffer = data.data();
        assert(spi_device_polling_transmit(spiDeviceHandle, &t)==ESP_OK);
    }

    void sendDMA(std::vector<std::byte> const& data) const{
        spi_transaction_t t;
        std::memset(&t, 0, sizeof(t));
        t.length = data.size()*sizeof(std::byte)*8;
        t.tx_buffer = data.data();
        assert(spi_device_queue_trans(spiDeviceHandle, &t, portMAX_DELAY) == ESP_OK);
    }

    void waitDMA(std::size_t messages) const{
        spi_transaction_t *rtrans;
        for(std::size_t i = 0; i < messages; ++i){
            assert(spi_device_get_trans_result(spiDeviceHandle, &rtrans, portMAX_DELAY) == ESP_OK);
        }
    }
};

};   // namespace esp
