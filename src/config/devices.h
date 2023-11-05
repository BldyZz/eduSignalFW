/**
 *	This file contains all configurations of the devices used in this project.
 */

/** Overview
*+----------+----------------------------------------------------+-----------------+
*|  Device  |                        Type                        | Connection Type |
*+----------+----------------------------------------------------+-----------------+
*| ADS1299  | ADC for EEG and biopotential measurement           | SPI             |
*| TSC2003  | Touch Screen Controller                            | I2C             |
*| BHI160   | IMU                                                | I2C             |
*| MAX30102 | Heart Rate Pulse Blood Oxygen Concentration Sensor | I2C             |
*| MCP3561  | Two/Four/Eight-Channel ADC                         | SPI             |
*| PCF8574  | IO-Expander                                        | I2C             |
*+----------+----------------------------------------------------+-----------------+
*/

#pragma once

#include "i2c.h"
#include "spi.h"

using address_t = std::uint8_t;

namespace config
{
	struct ADS1299
	{
		using Config = BoardSPIConfig;
		
		static constexpr size_t     ID							= 0;
		static constexpr size_t		CLOCK_SPEED                 = 1 * 100 * 1000;
		static constexpr size_t     CHANNEL_COUNT               = 4;
		static constexpr size_t     ECG_SAMPLES_IN_RINGBUFFER   = 10;
		static constexpr size_t     NOISE_SAMPLES_IN_RINGBUFFER = 10;
		static constexpr size_t		SPI_MAX_TRANSACTION_LENGTH  = 20; // Maximum length of one spi transaction in bytes.
		static constexpr gpio_num_t CS_PIN                      = GPIO_NUM_5; // Chip select
		static constexpr gpio_num_t RESET_PIN                   = GPIO_NUM_4; // System reset
		static constexpr gpio_num_t N_PDWN_PIN                  = GPIO_NUM_0; // Power-down - Active low
		static constexpr gpio_num_t N_DRDY_PIN                  = GPIO_NUM_36; // Data Ready Pin - Active low
		static constexpr uint8_t    SPI_MODE                    = 0x01;
		
	};

	struct TSC2003
	{
		using Config = I2C0_Config;

		static constexpr address_t  ADDRESS  = 0x48;
		static constexpr gpio_num_t NIRQ_PIN = GPIO_NUM_2;
	};

	struct BHI160
	{
		using Config = I2C0_Config;

		static constexpr size_t     ID                    = 1;
		static constexpr size_t     SAMPLES_IN_RINGBUFFER = 10;
		static constexpr gpio_num_t INTERRUPT_PIN         = GPIO_NUM_39;
		static constexpr address_t  ADDRESS               = 0x28;
	};

	struct MAX30102
	{
		using Config = I2C0_Config;

		static constexpr size_t    ID                     = 2;
		static constexpr address_t ADDRESS                = 0x57;
		static constexpr size_t    SAMPLES_IN_RING_BUFFER = 100;
	};

	struct MCP3561
	{
		using Config = BoardSPIConfig;

		static constexpr size_t     ID = 3;
		static constexpr address_t  ADDRESS                    = 0x1;
		static constexpr size_t		CLOCK_SPEED                = 1 * 100 * 1000;
		static constexpr size_t		SPI_MAX_TRANSACTION_LENGTH = 20; // Maximum length of one spi transaction in bytes.
		static constexpr gpio_num_t CS_PIN                     = GPIO_NUM_33; // Chip select
		static constexpr gpio_num_t IRQ_PIN                    = GPIO_NUM_34;
		static constexpr uint8_t    SPI_MODE                   = 0x00; 
	};

	struct PCFB574
	{
		using Config = I2C0_Config;
		static constexpr address_t ADDRESS = 0x20;
	};
}
