//
// Created by patrick on 8/12/22.
//
#pragma once

#include "driver/i2c.h"

#include <cstdint>
#include <span>
#include <vector>



namespace esp
{
	using byte = uint8_t;

	template<typename I2CConfig, std::uint8_t deviceAddress>
	struct i2cDevice
	{
		void read(
			byte const         registerAddress,
			std::size_t const          length,
			std::uint8_t* data)
		{
			std::uint8_t tempRegisterAddress = registerAddress;
			ESP_ERROR_CHECK(i2c_master_write_read_device(
				I2CConfig::Number,
				deviceAddress,
				&tempRegisterAddress,
				1,
				data,
				length,
				I2CConfig::TimeoutMS / portTICK_PERIOD_MS));
		}

		void readOnly(
			std::size_t const          length,
			std::uint8_t* data)
		{
			ESP_ERROR_CHECK(i2c_master_read_from_device(
				I2CConfig::Number,
				deviceAddress,
				data,
				length,
				I2CConfig::TimeoutMS / portTICK_PERIOD_MS));
		}

		void write(std::span<byte const> dataToWrite)
		{
			ESP_ERROR_CHECK(i2c_master_write_to_device(
				I2CConfig::Number,
				deviceAddress,
				reinterpret_cast<const std::uint8_t*>(dataToWrite.data()),
				dataToWrite.size(),
				I2CConfig::TimeoutMS / portTICK_PERIOD_MS));
		}
	};
}   // namespace esp