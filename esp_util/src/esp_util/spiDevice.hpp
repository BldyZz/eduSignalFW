//
// Created by patrick on 5/25/22.
//
#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "spiHost.hpp"
#include "transactionManager.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <vector>
#include <cassert>

namespace esp {
    template<typename SPIConfig, std::size_t MaxTransactions>
struct spiDevice {
    using CallbackType = void(*)(void);
private:
    spi_device_interface_config_t interfaceConfig;
    spi_device_handle_t           spiDeviceHandle;
    TransactionManager<spi_transaction_t, MaxTransactions> transactions;

public:
    explicit spiDevice(
      spiHost<SPIConfig> const&     host,
      std::size_t const  clockSpeed,
      gpio_num_t const   chipSelect,
      std::uint8_t const spiMode)
      : interfaceConfig{.mode = spiMode, .cs_ena_pretrans = 0,.clock_speed_hz = static_cast<int>(clockSpeed),  .spics_io_num = chipSelect, .queue_size = static_cast<int>(MaxTransactions+1),  .pre_cb = &spiDevice::preTransferCallback}
      , spiDeviceHandle{} {
        ESP_ERROR_CHECK(
          spi_bus_add_device(host.getHostDevice(), &interfaceConfig, &spiDeviceHandle));
    }

    template<typename F>
    void sendBlocking(std::span<std::byte const> package, F callback) {
        spi_transaction_t t{};
        t.user = &callback;
        t.length    = package.size_bytes() * 8;
        t.tx_buffer = package.data();
        assert(spi_device_polling_transmit(spiDeviceHandle, &t) == ESP_OK);
    }

    void sendBlocking(std::span<std::byte const> package) {
        spi_transaction_t t{};
        t.length    = package.size_bytes() * 8;
        t.tx_buffer = package.data();
        assert(spi_device_polling_transmit(spiDeviceHandle, &t) == ESP_OK);
    }
/*
    template<typename F>
    void sendBlocking(std::span<std::byte const> package, F callback, std::vector<std::byte> &returnData) {
        spi_transaction_t t{};
        t.user = &callback;
        t.length    = package.size_bytes() * 8;
        t.tx_buffer = package.data();
        assert(spi_device_polling_transmit(spiDeviceHandle, &t) == ESP_OK);
        returnData.resize(t.rxlength);
        std::memcpy(&returnData[0], &t.rx_data[0], returnData.size());
    }
*/
    template<typename F>
    void sendDMA(std::span<std::byte const> package, F callback) {
        spi_transaction_t& t = transactions.getFreeTransaction();
        t.user      = reinterpret_cast<void*>(CallbackType{callback});
        t.length    = package.size_bytes() * 8;
        t.tx_buffer = package.data();
        //fmt::print("RX:{}\tTX:{}\tFlags: {}\tCMD:{}\n", t.length, t.rxlength, t.flags, t.cmd);
        assert(spi_device_queue_trans(spiDeviceHandle, &t, portMAX_DELAY) == ESP_OK);
    }

    void waitDMA(std::size_t messages) {
        for(std::size_t i = 0; i < messages; ++i) {
            spi_transaction_t* rtrans;
            assert(spi_device_get_trans_result(spiDeviceHandle, &rtrans, portMAX_DELAY) == ESP_OK);
            transactions.releaseTransaction(rtrans);
        }
    }

    static void preTransferCallback(spi_transaction_t* t) {
        assert(t != nullptr);
        if(t->user != nullptr){
            CallbackType cb = reinterpret_cast<CallbackType>(t->user);
            cb();
        }
    }
};

};   // namespace esp
