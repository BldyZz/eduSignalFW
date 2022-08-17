//
// Created by patrick on 8/12/22.
//
#pragma once

#include "driver/i2c.h"
#include "driver/gpio.h"

namespace esp {
template<typename I2CConfig>
struct i2cMaster {
private:
public:
    explicit i2cMaster() {

        i2c_config_t busConfig = {
          .mode          = I2C_MODE_MASTER,
          .sda_io_num    = I2CConfig::SDAPin,
          .scl_io_num    = I2CConfig::SCLPin,
          .sda_pullup_en = I2CConfig::SDAPullup,
          .scl_pullup_en = I2CConfig::SCLPullup,
          .master{.clk_speed = I2CConfig::Frequency},
        };
        int i2c_master_port = I2CConfig::Number;
        i2c_param_config(i2c_master_port, &busConfig);
        ESP_ERROR_CHECK(i2c_driver_install(
          i2c_master_port,
          busConfig.mode,
          I2CConfig::RXBufferSize,
          I2CConfig::TXBufferSize,
          0));

        gpio_set_direction(I2CConfig::PowerPin, GPIO_MODE_OUTPUT);
        gpio_set_level(I2CConfig::PowerPin, 1);
        gpio_set_level(I2CConfig::PowerPin, 0);
    }
};
}   // namespace esp