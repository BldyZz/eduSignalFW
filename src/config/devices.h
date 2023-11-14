/**
 *	This file contains all configurations of the devices used in this project.
 */

/** Overview
*	+----------+----------------------------------------------------+-----------------+
*	|  Device  |                        Type                        | Connection Type |
*	+----------+----------------------------------------------------+-----------------+
*	| ADS1299  | ADC for EEG and biopotential measurement           | SPI             |
*	| TSC2003  | Touch Screen Controller                            | I2C             |
*	| BHI160   | IMU                                                | I2C             |
*	| MAX30102 | Heart Rate Pulse Blood Oxygen Concentration Sensor | I2C             |
*	| MCP3561  | Two/Four/Eight-Channel ADC                         | SPI             |
*	| PCF8574  | IO-Expander                                        | I2C             |
*	+----------+----------------------------------------------------+-----------------+
*/

#pragma once

#include "i2c.h"
#include "spi.h"

using address_t = unsigned char;

namespace config
{
	using ascii_t = char;

	static constexpr float DURATION_OF_MEASUREMENT = 1.f;
	static constexpr float OVERFLOW_SAFETY_FACTOR = 1.2f;

	struct ADS1299
	{
		using Config = BoardSPIConfig;

		static constexpr size_t  SAMPLE_RATE         = 250; // in SPS
		// BDF Info
		static constexpr size_t  CHANNEL_COUNT        = 4;
		static constexpr ascii_t LABEL[]              = "ECG";
		static constexpr ascii_t TRANSDUCER_TYPE[]    = "ADC";
		static constexpr ascii_t PHYSICAL_DIMENSION[] = "uV";
		static constexpr int32_t PHYSICAL_MINIMUM     = -200'000;
		static constexpr int32_t PHYSICAL_MAXIMUM     = 200'000;
		static constexpr int32_t DIGITAL_MINIMUM      = -200'000;
		static constexpr int32_t DIGITAL_MAXIMUM      = 200'000;
		static constexpr ascii_t PRE_FILTERING[]      = "None";
		static constexpr size_t  NODES_IN_BDF_RECORD = SAMPLE_RATE * DURATION_OF_MEASUREMENT;

		static constexpr size_t     ECG_SAMPLES_IN_RING_BUFFER   = NODES_IN_BDF_RECORD * OVERFLOW_SAFETY_FACTOR;

		static constexpr size_t     CLOCK_SPEED                 = 1 * 100 * 1000;
		static constexpr size_t     NOISE_SAMPLES_IN_RING_BUFFER = 1;
		static constexpr size_t     SPI_MAX_TRANSACTION_LENGTH  = 20;          // Maximum length of one spi transaction in bytes.
		static constexpr gpio_num_t CS_PIN                      = GPIO_NUM_5;  // Chip select
		static constexpr gpio_num_t RESET_PIN                   = GPIO_NUM_4;  // System reset
		static constexpr gpio_num_t N_PDWN_PIN                  = GPIO_NUM_0;  // Power-down - Active low
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

		static constexpr uint16_t SAMPLE_RATE = 50; // in SPS

		// BDF Info
		static constexpr size_t  CHANNEL_COUNT = 4; // X, Y, Z, Status
		static constexpr ascii_t LABEL[] = "Acceleration";
		static constexpr ascii_t TRANSDUCER_TYPE[] = "Accelerometer";
		static constexpr ascii_t PHYSICAL_DIMENSION[] = "m/s^2";
		static constexpr int32_t PHYSICAL_MINIMUM = -200'000;
		static constexpr int32_t PHYSICAL_MAXIMUM = 200'000;
		static constexpr int32_t DIGITAL_MINIMUM = -200'000;
		static constexpr int32_t DIGITAL_MAXIMUM = 200'000;
		static constexpr ascii_t PRE_FILTERING[] = "None";
		static constexpr size_t  NODES_IN_BDF_RECORD = SAMPLE_RATE * DURATION_OF_MEASUREMENT;

		static constexpr uint16_t   LATENCY               = 40; // in ms
		static constexpr uint16_t   DYNAMIC_RANGE         = 0;  // (Default = 0)
		static constexpr uint16_t   SENSITIVITY           = 0;  // (Default = 0)
		static constexpr size_t     SAMPLES_IN_RINGBUFFER = NODES_IN_BDF_RECORD * OVERFLOW_SAFETY_FACTOR;
		static constexpr gpio_num_t INTERRUPT_PIN         = GPIO_NUM_39;
		static constexpr address_t  ADDRESS               = 0x28;
	};

	struct MAX30102
	{
		using Config = I2C0_Config;

		static constexpr size_t    SAMPLE_RATE            = 100; // in SPS

		// BDF Info
		static constexpr size_t  CHANNEL_COUNT = 2; // Red, Infrared
		static constexpr ascii_t LABEL[] = "Pulse Oxi Meter";
		static constexpr ascii_t TRANSDUCER_TYPE[] = "oxioxi";
		static constexpr ascii_t PHYSICAL_DIMENSION[] = "r/ir"; // @TODO: Add real dimensions
		static constexpr int32_t PHYSICAL_MINIMUM = -200'000;
		static constexpr int32_t PHYSICAL_MAXIMUM = 200'000;
		static constexpr int32_t DIGITAL_MINIMUM = -200'000;
		static constexpr int32_t DIGITAL_MAXIMUM = 200'000;
		static constexpr ascii_t PRE_FILTERING[] = "None";
		static constexpr size_t  NODES_IN_BDF_RECORD = SAMPLE_RATE * DURATION_OF_MEASUREMENT;

		static constexpr address_t ADDRESS                = 0x57;
		static constexpr size_t    SAMPLES_IN_RING_BUFFER = NODES_IN_BDF_RECORD * OVERFLOW_SAFETY_FACTOR;
	};

	struct MCP3561
	{
		using Config = BoardSPIConfig;

		static constexpr size_t     ID                         = 3;
		static constexpr size_t     CHANNEL_COUNT              = 1;

		static constexpr address_t  ADDRESS                    = 0x1;
		static constexpr size_t     CLOCK_SPEED                = 1 * 100 * 1000;
		static constexpr size_t     SPI_MAX_TRANSACTION_LENGTH = 20;          // Maximum length of one spi transaction in bytes.
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
