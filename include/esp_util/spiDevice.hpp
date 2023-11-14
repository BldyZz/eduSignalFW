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
#include <cassert>

namespace esp
{
	using byte = uint8_t;

	template<typename SPIConfig, std::size_t MaxTransactions>
	struct spiDevice
	{
		using CallbackType = void(*)(void);
	private:
		spi_device_interface_config_t interfaceConfig;
		spi_device_handle_t           spiDeviceHandle;
		TransactionManager<spi_transaction_t, MaxTransactions> transactions;

	public:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
		explicit spiDevice(
			spiHost<SPIConfig> const& host,
			std::size_t const  clockSpeed,
			gpio_num_t const   chipSelect,
			std::uint8_t const spiMode)
			: interfaceConfig{.mode = spiMode, .cs_ena_pretrans = 0,.clock_speed_hz = static_cast<int>(clockSpeed),  .spics_io_num = chipSelect, .queue_size = static_cast<int>(MaxTransactions + 1),  .pre_cb = &spiDevice::preTransferCallback}
			, spiDeviceHandle{}
		{
			ESP_ERROR_CHECK(
				spi_bus_add_device(host.getHostDevice(), &interfaceConfig, &spiDeviceHandle));
		}
#pragma GCC diagnostic pop

		template<typename F>
		void sendBlocking(std::span<byte const> package, F callback)
		{
			spi_transaction_t t{};
			t.user = &callback;
			t.length = package.size_bytes() * 8;
			t.tx_buffer = package.data();
			assert(spi_device_polling_transmit(spiDeviceHandle, &t) == ESP_OK);
		}

		void sendBlocking(std::span<byte const> package)
		{
			spi_transaction_t t{};
			t.length = package.size_bytes() * 8;
			t.tx_buffer = package.data();
			assert(spi_device_polling_transmit(spiDeviceHandle, &t) == ESP_OK);
		}

		template<typename F, std::size_t arraySize>
		void sendBlocking(std::span<byte const> package, F callback, std::array<byte, arraySize>& returnData)
		{
			spi_transaction_t t{};
			t.user = &callback;
			t.length = package.size_bytes() * 8;
			t.rx_buffer = returnData.data();
			t.tx_buffer = package.data();
			assert(spi_device_polling_transmit(spiDeviceHandle, &t) == ESP_OK);
			assert(package.size_bytes() == returnData.size());
		}

		template<std::size_t arraySize>
		void sendBlocking(std::span<byte const> package, std::array<byte, arraySize>& returnData)
		{
			spi_transaction_t t{};
			t.length = package.size_bytes() * 8;
			t.tx_buffer = package.data();
			t.rx_buffer = returnData.data();
			assert(spi_device_polling_transmit(spiDeviceHandle, &t) == ESP_OK);
			assert(package.size_bytes() == returnData.size());
		}

		template<typename F>
		void sendDMA(std::span<byte const> package, F callback)
		{
			spi_transaction_t& t = transactions.getFreeTransaction();
			t.user = reinterpret_cast<void*>(CallbackType{callback});
			t.length = package.size_bytes() * 8;
			t.tx_buffer = package.data();
			//fmt::print("RX:{}\tTX:{}\tFlags: {}\tCMD:{}\n", t.length, t.rxlength, t.flags, t.cmd);
			assert(spi_device_queue_trans(spiDeviceHandle, &t, portMAX_DELAY) == ESP_OK);
		}

		template<typename F>
		void sendDMACopy(std::span<byte const> package, F callback)
		{
			assert(package.size() < 5);

			spi_transaction_t& t = transactions.getFreeTransaction();
			std::memcpy(t.tx_data, package.data(), package.size_bytes());
			t.flags = SPI_TRANS_USE_TXDATA;
			t.tx_buffer = nullptr;
			t.user = reinterpret_cast<void*>(CallbackType{callback});
			t.length = package.size_bytes() * 8;
			//fmt::print("RX:{}\tTX:{}\tFlags: {}\tCMD:{}\n", t.length, t.rxlength, t.flags, t.cmd);
			assert(spi_device_queue_trans(spiDeviceHandle, &t, portMAX_DELAY) == ESP_OK);
		}

		void waitDMA(std::size_t messages)
		{
			for(std::size_t i = 0; i < messages; ++i)
			{
				spi_transaction_t* rtrans;
				assert(spi_device_get_trans_result(spiDeviceHandle, &rtrans, portMAX_DELAY) == ESP_OK);
				transactions.releaseTransaction(rtrans);
			}
		}

		static void preTransferCallback(spi_transaction_t* t)
		{
			assert(t != nullptr);
			if(t->user != nullptr)
			{
				CallbackType cb = reinterpret_cast<CallbackType>(t->user);
				cb();
			}
		}
	};

};   // namespace esp
