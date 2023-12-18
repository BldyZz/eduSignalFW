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

#include <cmath>

#define INT24_MAX 8'388'607
#define INT24_MIN -8'388'608

using address_t = unsigned char;

namespace config
{
	using ascii_t = char;

	static constexpr float DURATION_OF_MEASUREMENT = 0.4f;
	static constexpr float OVERFLOW_SAFETY_FACTOR = 2.0f;

	template<typename T>
	consteval size_t ceil_to_power_2(T value)
	{
		return 1 << static_cast<size_t>(std::ceil(std::log2(value)));
	}

	consteval long sample_rate_to_us_with_deviation(float sampleRate)
	{
		constexpr long deviation_delta = 0;
		return static_cast<long>(1'000'000.f * (1 / sampleRate)) + deviation_delta;
	}

	consteval long sample_rate_to_ms(float sampleRate)
	{
		return static_cast<long>(1'000.f / sampleRate);
	}
	
	struct ADS1299
	{
		using Config = BoardSPIConfig;

		static constexpr size_t  SAMPLE_RATE         = 125; // in SPS
		// BDF Info
		static constexpr size_t      CHANNEL_COUNT                      = 4;
		inline static const ascii_t* LABELS[CHANNEL_COUNT]              = {"ECG Ch1", "ECG Ch2", "ECG Ch3", "ECG Ch4"};
		static constexpr ascii_t     TRANSDUCER_TYPE[]                  = "ADC";
		inline static const ascii_t* PHYSICAL_DIMENSIONS[CHANNEL_COUNT] = {"uV", "uV", "uV", "uV"};
		static constexpr int32_t     PHYSICAL_MINIMUM                   = INT24_MIN;
		static constexpr int32_t     PHYSICAL_MAXIMUM                   = INT24_MAX;
		static constexpr int32_t     DIGITAL_MINIMUM                    = INT24_MIN;
		static constexpr int32_t     DIGITAL_MAXIMUM                    = INT24_MAX;
		static constexpr ascii_t     PRE_FILTERING[]                    = "None";
		static constexpr size_t      NODES_IN_BDF_RECORD                = SAMPLE_RATE * DURATION_OF_MEASUREMENT;

		static constexpr size_t     ECG_SAMPLES_IN_RING_BUFFER   = ceil_to_power_2(NODES_IN_BDF_RECORD * OVERFLOW_SAFETY_FACTOR);

		static constexpr size_t     CLOCK_SPEED                  = 1 * 100 * 1000;
		static constexpr size_t     NOISE_SAMPLES_IN_RING_BUFFER = 1;
		static constexpr size_t     SPI_MAX_TRANSACTION_LENGTH   = 20;          // Maximum length of one spi transaction in bytes.
		static constexpr gpio_num_t CS_PIN                       = GPIO_NUM_5;  // Chip select
		static constexpr gpio_num_t RESET_PIN                    = GPIO_NUM_4;  // System reset
		static constexpr gpio_num_t N_PDWN_PIN                   = GPIO_NUM_0;  // Power-down - Active low
		static constexpr gpio_num_t N_DRDY_PIN                   = GPIO_NUM_36; // Data Ready Pin - Active low
		static constexpr uint8_t    SPI_MODE                     = 0x01;
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
		static constexpr size_t		 CHANNEL_COUNT                      = 4; // X, Y, Z, Status
		inline static const ascii_t* LABELS[CHANNEL_COUNT]              = {"Acceleration X", "Acceleration Y", "Acceleration Z", "Acceleration Status"};
		static constexpr ascii_t	 TRANSDUCER_TYPE[]                  = "Accelerometer";
		inline static const ascii_t* PHYSICAL_DIMENSIONS[CHANNEL_COUNT] = {"m/s^2", "m/s^2", "m/s^2", "Accuracy"};
		static constexpr int32_t	 PHYSICAL_MINIMUM                   = INT24_MIN;
		static constexpr int32_t	 PHYSICAL_MAXIMUM                   = INT24_MAX;
		static constexpr int32_t	 DIGITAL_MINIMUM                    = INT24_MIN;
		static constexpr int32_t	 DIGITAL_MAXIMUM                    = INT24_MAX;
		static constexpr ascii_t	 PRE_FILTERING[]                    = "None";
		static constexpr size_t		 NODES_IN_BDF_RECORD                = SAMPLE_RATE * DURATION_OF_MEASUREMENT;

		static constexpr uint16_t   LATENCY                = 40; // in ms
		static constexpr uint16_t   DYNAMIC_RANGE          = 0;  // (Default = 0)
		static constexpr uint16_t   SENSITIVITY            = 0;  // (Default = 0)
		static constexpr size_t     SAMPLES_IN_RING_BUFFER = ceil_to_power_2(NODES_IN_BDF_RECORD * OVERFLOW_SAFETY_FACTOR);
		static constexpr gpio_num_t INTERRUPT_PIN          = GPIO_NUM_39;
		static constexpr address_t  ADDRESS                = 0x28;
	};

	struct MAX30102
	{
		using Config = I2C0_Config;

		static constexpr size_t    SAMPLE_RATE            = 100; // in SPS

		// BDF Info
		static constexpr size_t  CHANNEL_COUNT             = 2; // Red, Infrared
		inline static const ascii_t* LABELS[]              = {"Oxi Red", "Oxi InfraRed"};
		static constexpr ascii_t TRANSDUCER_TYPE[]         = "oxi";
		inline static const ascii_t* PHYSICAL_DIMENSIONS[] = {"Count", "Count"}; // @TODO: Add real dimensions
		static constexpr int32_t PHYSICAL_MINIMUM          = INT24_MIN;
		static constexpr int32_t PHYSICAL_MAXIMUM          = INT24_MAX;
		static constexpr int32_t DIGITAL_MINIMUM           = INT24_MIN;
		static constexpr int32_t DIGITAL_MAXIMUM           = INT24_MAX;
		static constexpr ascii_t PRE_FILTERING[]           = "None";
		static constexpr size_t  NODES_IN_BDF_RECORD       = SAMPLE_RATE * DURATION_OF_MEASUREMENT;

		static constexpr address_t ADDRESS                = 0x57;
		static constexpr size_t    SAMPLES_IN_RING_BUFFER = ceil_to_power_2(NODES_IN_BDF_RECORD * OVERFLOW_SAFETY_FACTOR);
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

	struct BDF
	{
		static constexpr size_t OVERALL_CHANNELS = ADS1299::CHANNEL_COUNT + BHI160::CHANNEL_COUNT + MAX30102::CHANNEL_COUNT;
		static constexpr size_t SEND_STACK_SIZE = ADS1299::CHANNEL_COUNT * ADS1299::NODES_IN_BDF_RECORD + 
											      BHI160::CHANNEL_COUNT * BHI160::NODES_IN_BDF_RECORD +
											      MAX30102::CHANNEL_COUNT * MAX30102::NODES_IN_BDF_RECORD;
	};
}
