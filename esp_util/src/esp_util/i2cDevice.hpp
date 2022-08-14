//
// Created by patrick on 8/12/22.
//
#pragma once

#include "driver/i2c.h"

#include <cstdint>
#include <span>
#include <vector>

namespace esp {
template<typename I2CConfig, std::uint8_t deviceAddress>
struct i2cDevice {
    void read(
      std::uint8_t const         registerAddress,
      std::size_t const          length,
      std::uint8_t* data) {
        ESP_ERROR_CHECK(i2c_master_write_read_device(
          I2CConfig::Number,
          deviceAddress,
          &registerAddress,
          1,
          data,
          length,
          I2CConfig::TimeoutMS / portTICK_PERIOD_MS));
    }
    void write(std::span<std::byte const> dataToWrite) {
        ESP_ERROR_CHECK(i2c_master_write_to_device(
          I2CConfig::Number,
          deviceAddress,
          reinterpret_cast<const std::uint8_t* >(dataToWrite.data()),
          dataToWrite.size(),
          I2CConfig::TimeoutMS / portTICK_PERIOD_MS));
    }
};
}   // namespace esp