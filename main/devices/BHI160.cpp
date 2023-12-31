#include "../util/utils.h"
#include "../util/time.h"

#include "BHI160.hpp"

#include <algorithm>

#include "BHI160_Firmware.hpp"

#include <cstdio>
#include <cstring>

namespace device
{
	enum class BHI160::State : util::byte
	{
		Reset,
		WaitForInterrupt1,
		FirmwareUpload,
		ModeSwitch,
		WaitForInterrupt2,
		Configuration,
		Idle,
		GetData
	};

	struct BHI160::Command
	{
		enum : util::byte
		{
			Host_Upload_Enable = 0x02,
			CPU_Run_Request = 0x01,
			FLUSH_ALL = 0xFF,
		};
	};

	struct BHI160::Register
	{
		enum : util::byte
		{
			Chip_Control = 0x34,
			Reset_Request = 0x9B,
			Upload_Data = 0x96,
			Upload_Address_0 = 0x94,
			Upload_Address_1 = 0x95,
			Upload_CRC = 0x97,
			Chip_Status = 0x37,
			Host_Status = 0x35,
			RAM_Version = 0x72,
			ROM_Version = 0x70,
			Bytes_Remaining = 0x38,
			Buffer_out = 0x00,
			FIFO_Flush = 0x32,
			Parameter_Page_Select = 0x54,
			Parameter_Read_Buffer = 0x3B,
			Parameter_Write_Buffer = 0x5C,
			Parameter_Request = 0x64,
		};
	};

	struct BHI160::Event
	{
		enum : util::byte
		{
			TimestampLSWWakeUp = 246,
			TimestampLSW = 252,
			Accelerometer = 1,
			AccelerometerWakeUp = 33,
			TimestampMSWWakeUp = 247,
			TimestampMSW = 253,
			MetaWakeUp = 248,
			Meta = 254,
		};
	};

	BHI160::BHI160()
		: _acceleration{},
		  _timestamp(0),
		  _nextTime(timepoint_t::clock::now()),
		  _bytesInFIFO(0),
		  _state(State::Reset)
	{
	}

	void BHI160::Init()
	{
		// Create ring buffer
		_buffer = mem::RingBuffer(&_mutexBuffer, _acceleration, sizeof(acceleration_t), std::size(_acceleration), config::BHI160::CHANNEL_COUNT);

		gpio_set_direction(config::BHI160::INTERRUPT_PIN, GPIO_MODE_INPUT);

		PRINTI("[BHI160:]", "Resetting...\n");
		Reset();

		if(gpio_get_level(config::BHI160::INTERRUPT_PIN))
		{
			StartRAMPatch();
			_state = State::FirmwareUpload;
			PRINTI("[BHI160:]", "Uploading firmware...\n");
		}

		UploadFirmware();
		StartCPU();
		if(gpio_get_level(config::BHI160::INTERRUPT_PIN))
		{
			PRINTI("[BHI160:]", "Configuring device...\n");
			PrintVersionAndStatus();
			_state = State::Configuration;
		}

		ConfigureDevices();

		PRINTI("[BHI160:]", "Initialization successful.\n");
	}

	void BHI160::HandleData(std::span<util::byte> package)
	{
		if(package.empty()) return;

		switch(package[0])
		{
		case Event::TimestampLSWWakeUp:
		case Event::TimestampLSW:
		{
			std::uint16_t TimestampLSW{};
			std::memcpy(&TimestampLSW, &package[1], 2);
			_timestamp = (_timestamp & 0xFF00) | TimestampLSW;
			HandleData(std::span{package.begin() + 3, package.end()});
		}
		break;
		case Event::TimestampMSWWakeUp:
		case Event::TimestampMSW:
		{
			std::uint16_t TimestampMSW{};
			std::memcpy(&TimestampMSW, &package[1], 2);
			_timestamp = (_timestamp & 0xFF) | (TimestampMSW << 8);
			HandleData(std::span{package.begin() + 3, package.end()});
		}
		break;
		case Event::Accelerometer:
		case Event::AccelerometerWakeUp:
		{
#if 0
			*static_cast<acceleration_t*>(_buffer.CurrentWrite()) = acceleration_t
			{
				.X = mem::int24_t(static_cast<int16_t>(package[5] | package[6] << 8)), // X
				.Y = mem::int24_t(static_cast<int16_t>(package[3] | package[4] << 8)), // Y
				.Z = mem::int24_t(static_cast<int16_t>(package[1] | package[2] << 8)), // Z
				.status = mem::int24_t(static_cast<int16_t>(package[7])),                  // Status
			};
#endif
			_buffer.WriteAdvance();
			HandleData(std::span{package.begin() + 8, package.end()});
			_nextTime = timepoint_t::clock::now() + std::chrono::milliseconds(config::sample_rate_to_us_with_deviation(config::BHI160::SAMPLE_RATE));
		}
		break;

		case Event::Meta:
		case Event::MetaWakeUp:
			HandleData(std::span{package.begin() + 4, package.end()});
			break;
		default:
			break;
		}
	}

	void BHI160::Reset()
	{
		static constexpr util::byte resetPackage[] = {Register::Reset_Request, 0x01};
		this->write(util::to_span(resetPackage));
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	void BHI160::StartRAMPatch()
	{
		static constexpr util::byte ramPatchMechanism[] = {Register::Chip_Control, Command::Host_Upload_Enable};
		this->write(util::to_span(ramPatchMechanism));
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	void BHI160::UploadFirmware()
	{
		static constexpr util::byte uploadAddress0[] = {Register::Upload_Address_0, util::PADDING_BYTE, util::PADDING_BYTE};
		this->write(util::to_span(uploadAddress0));
		std::size_t counter = 0;
		vTaskDelay(pdMS_TO_TICKS(10));
		auto iterBegin{BHI160_Firmware.begin()};
		while(iterBegin < BHI160_Firmware.end())
		{
			static constexpr size_t BYTES_TO_WRITE = 16;
			util::byte buffer[BYTES_TO_WRITE + 1];

			buffer[0] = Register::Upload_Data;
			if(iterBegin + BYTES_TO_WRITE > BHI160_Firmware.end())
			{
				auto bytesUntilEnd{(BHI160_Firmware.end() - iterBegin)};
				counter = counter + bytesUntilEnd;
				std::memcpy(&buffer[1], iterBegin, bytesUntilEnd);
				this->write(std::span{buffer, buffer + bytesUntilEnd + 1});
			} else
			{
				counter += BYTES_TO_WRITE;
				std::memcpy(&buffer[1], iterBegin, BYTES_TO_WRITE);
				this->write(buffer);
			}
			iterBegin += BYTES_TO_WRITE;
		}

		PRINTI("[BHI160:]", "%u Bytes written...\n", counter);

		vTaskDelay(pdMS_TO_TICKS(10)); 

		std::array<std::uint8_t, 4> registerCRC;
		this->read(Register::Upload_CRC, registerCRC.size(), &registerCRC[0]);
		std::ranges::reverse(registerCRC);
		PRINTI("[BHI160:]", "Register CRC: 0x%04lx, Firmware CRC: 0x%04lx", *reinterpret_cast<uint32_t*>(registerCRC.data()),  BHI160_Firmware_CRC);
		uint16_t registerAddress;
		this->read(Register::Upload_Address_0, sizeof(registerAddress), reinterpret_cast<util::byte*>(&registerAddress));
		//this->read(Register::Upload_Address_1, 1, &RegisterAddress[1]);
		//fmt::print("[BHI160:] Register Address 0x{:02x}\n", registerAddress);

	}

	void BHI160::StartCPU()
	{
		static constexpr util::byte cpuRunPackage[] = {Register::Chip_Control, Command::CPU_Run_Request};
		this->write(util::to_span(cpuRunPackage));
		//Keep read after write here, otherwise Chip wont boot properly!!!
		std::uint8_t ChipControl;
		this->read(Register::Chip_Control, 1, &ChipControl);
		PRINTI("[BHI160:]", "ChipControl: %02X\n", ChipControl);
	}

	void BHI160::ConfigureDevices()
	{
		//TODO: configure sensors, meta events, fifo buffers and host interrupt
			//Disable FIFO Watermark
			//Enable Accelerometer
			//Configure Accelerometer for 50Hz and 40ms Latency
		static constexpr std::uint8_t  parameterPage = 3;
		static constexpr util::byte parameterPageSelectionPackage[] = {Register::Parameter_Page_Select, 0b1111 & parameterPage};
		this->write(util::to_span(parameterPageSelectionPackage));
		vTaskDelay(pdMS_TO_TICKS(10));

		static constexpr std::uint8_t  sensorID = 65;

		// Register - Sample Rate 16 Bit - Latency ms 16Bit
		static constexpr util::byte sensorConfig[] = {
		  Register::Parameter_Write_Buffer,
		  (config::BHI160::SAMPLE_RATE & 0xFF),
		  (config::BHI160::SAMPLE_RATE & 0xFF00) >> BYTES_TO_BITS(1),
		  (config::BHI160::LATENCY & 0xFF),
		  (config::BHI160::LATENCY & 0xFF00) >> BYTES_TO_BITS(1),
		  (config::BHI160::DYNAMIC_RANGE & 0xFF),
		  (config::BHI160::DYNAMIC_RANGE & 0xFF00) >> BYTES_TO_BITS(1),
		  (config::BHI160::SENSITIVITY & 0xFF),
		  (config::BHI160::SENSITIVITY & 0xFF00) >> BYTES_TO_BITS(1)};
		this->write(util::to_span(sensorConfig));
		vTaskDelay(pdMS_TO_TICKS(10));
		std::uint8_t rxData[8];
		this->read(Register::Parameter_Write_Buffer, util::total_size(rxData), rxData);

		static constexpr util::byte parameterRequest[] = {Register::Parameter_Request, 0x80 | sensorID};
		this->write(util::to_span(parameterRequest));

		//Enable other Step counter with 0 latency
	}

	void BHI160::GetRemainingFIFOSize()
	{
		std::uint16_t rxData;
		this->read(Register::Bytes_Remaining, util::total_size(rxData), reinterpret_cast<util::byte*>(&rxData));

		std::memcpy(&_bytesInFIFO, &rxData, util::total_size(rxData));
		//fmt::print("BHI160: Bytes remaining: {}\n", _bytesInFIFO);
	}

	void BHI160::GetData()
	{
		if(_bytesInFIFO == 0)
		{
			GetRemainingFIFOSize();
		}

		static constexpr uint16_t MAXIMUM_BUFFER_SIZE = 50;
		util::byte rxData[MAXIMUM_BUFFER_SIZE];

		std::size_t bytesToSend{};
		if(_bytesInFIFO > MAXIMUM_BUFFER_SIZE)
		{
			bytesToSend = MAXIMUM_BUFFER_SIZE;
			_bytesInFIFO = _bytesInFIFO - MAXIMUM_BUFFER_SIZE;
		} else
		{
			bytesToSend = _bytesInFIFO;
			_bytesInFIFO = 0;
			_state = State::Idle;
		}
		this->read(Register::Buffer_out, bytesToSend, rxData);
		const std::span printSpan(rxData, rxData + _bytesInFIFO);
		//fmt::print("BHI160: Buffer data: {:#4x}\n", fmt::join(std::as_bytes(printSpan), ", "));

		static constexpr util::byte flushSensorPackage[] = {Register::FIFO_Flush, 35};
		auto span = util::to_span(flushSensorPackage);
		this->write(span);

		//std::printf("span:");
		//for(auto i : printSpan)
		//{
		//	std::printf(" %x", i);
		//}
		//std::printf("\n");

		HandleData(printSpan);
	}

	void BHI160::InsertPadding()
	{
#if 0
		*static_cast<acceleration_t*>(_buffer.CurrentWrite()) = acceleration_t
		{
			.X = mem::int24_t(static_cast<int32_t>(0)),      // X
			.Y = mem::int24_t(static_cast<int32_t>(0)),      // Y
			.Z = mem::int24_t(static_cast<int32_t>(0)),      // Z
			.status = mem::int24_t(static_cast<int32_t>(0)), // Status
		};
#endif

		_buffer.WriteAdvance();
	}

	void BHI160::PrintVersionAndStatus()
	{
		std::uint16_t RAMVersion;
		this->read(Register::RAM_Version, sizeof(RAMVersion), reinterpret_cast<util::byte*>(&RAMVersion));
		PRINTI("[BHI160:]", "RAM Version %02X\n", RAMVersion);

		std::uint16_t ROMVersion;
		this->read(Register::ROM_Version, sizeof(ROMVersion), reinterpret_cast<util::byte*>(&ROMVersion));
		ROMVersion = util::byte_swap(ROMVersion);
		PRINTI("[BHI160:]", "ROM Version %02X\n", ROMVersion);

		util::byte ChipStatus;
		this->read(Register::Chip_Status, 1, &ChipStatus);
		PRINTI("[BHI160:]", "ChipStatus: %02X\n", ChipStatus);
	}

	bool BHI160::IsReady() const
	{
		return _state >= State::Idle;
	}

	mem::RingBuffer* BHI160::RingBuffer()
	{
		if(!_buffer.IsValid())
		{
			PRINTI("[BHI160:]", "Ring buffer was not initialized!\n");
		}
		return &_buffer;
	}

	bool BHI160::HasData() const
	{
		return (gpio_get_level(config::BHI160::INTERRUPT_PIN) == 1) | _bytesInFIFO;
	}
}
