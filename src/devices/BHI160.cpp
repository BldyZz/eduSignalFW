#include "../util/utils.h"

#include "BHI160.hpp"

#include <bit>

#include "BHI160_Firmware.hpp"

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
			Chip_Control           = 0x34,
			Reset_Request          = 0x9B,
			Upload_Data            = 0x96,
			Upload_Address_0       = 0x94,
			Upload_Address_1       = 0x95,
			Upload_CRC             = 0x97,
			Chip_Status            = 0x37,
			Host_Status            = 0x35,
			RAM_Version            = 0x72,
			ROM_Version            = 0x70,
			Bytes_Remaining        = 0x38,
			Buffer_out             = 0x00,
			FIFO_Flush             = 0x32,
			Parameter_Page_Select  = 0x54,
			Parameter_Read_Buffer  = 0x3B,
			Parameter_Write_Buffer = 0x5C,
			Parameter_Request      = 0x64,
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
		: _acceleration{}, _timestamp(0), _bytesInFIFO(0), _state(State::Reset)
	{
	}

	void BHI160::Init()
	{
		_state = State::Reset;
		_buffer = mem::createStaticRingBuffer(_acceleration, &_mutexBuffer);
		fmt::print("[BHI160:] Initialization successful.\n");
		gpio_set_direction(config::BHI160::INTERRUPT_PIN, GPIO_MODE_INPUT);
		while(!IsReady())
		{
			Handler();
		}
	}

	void BHI160::HandleData(std::span<util::byte> package)
	{
		if(package.empty())
		{
			return;
		}
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
			const std::int16_t tempZ      = package[1] | package[2] << 8;
			const std::int16_t tempY      = package[3] | package[4] << 8;
			const std::int16_t tempX      = package[5] | package[6] << 8;
			const std::uint8_t tempStatus = package[7];
			mem::write(&_buffer, acceleration_t
			{
			   .X = tempX,
			   .Y = tempY,
			   .Z = tempZ,
			   .status = tempStatus,
			});
			HandleData(std::span{package.begin() + 8, package.end()});
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
	}

	void BHI160::StartRAMPatch()
	{
		static constexpr util::byte ramPatchMechanism[] = {Register::Chip_Control, Command::Host_Upload_Enable};
		this->write(util::to_span(ramPatchMechanism));
	}

	void BHI160::UploadFirmware()
	{
		static constexpr util::byte uploadAddress0[] = {Register::Upload_Address_0, util::PADDING_BYTE, util::PADDING_BYTE};
		this->write(util::to_span(uploadAddress0));
		std::size_t counter = 0;

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

		fmt::print("[BHI160:] {} Bytes written...\n", counter);

		vTaskDelay(pdMS_TO_TICKS(10)); // Evil delay, but only necessary on firmware upload, so it doesn't matter.

		uint32_t registerCRC;
		this->read(Register::Upload_CRC, sizeof(registerCRC), reinterpret_cast<util::byte*>(&registerCRC));
		registerCRC = util::byte_swap(registerCRC);
		fmt::print("[BHI160:] Register CRC: 0x{:02x}, Firmware CRC: 0x{:x}\n", registerCRC, BHI160_Firmware_CRC);

		uint16_t registerAddress;
		this->read(Register::Upload_Address_0, sizeof(registerAddress), reinterpret_cast<util::byte*>(&registerAddress));
		//this->read(Register::Upload_Address_1, 1, &RegisterAddress[1]);
		fmt::print("[BHI160:] Register Address 0x{:02x}\n", registerAddress);

	}

	void BHI160::StartCPU()
	{
		static constexpr util::byte cpuRunPackage[] = {Register::Chip_Control, Command::CPU_Run_Request};
		this->write(util::to_span(cpuRunPackage));
		//Keep read after write here, otherwise Chip wont boot properly!!!
		std::uint8_t ChipControl;
		this->read(Register::Chip_Control, 1, &ChipControl);
		fmt::print("[BHI160:] ChipControl: 0x{:02x}\n", ChipControl);
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


		static constexpr std::uint16_t sampleRate = 50;
		static constexpr std::uint16_t latency = 40;
		static constexpr std::uint16_t dynamicRange = 0;
		static constexpr std::uint16_t sensitivity = 0;
		static constexpr std::uint8_t  sensorID = 65;


		static constexpr util::byte sensorConfig[] = {
		  Register::Parameter_Write_Buffer,
		  (sampleRate & 0xFF),
		  (sampleRate & 0xFF00) >> 8,
		  (latency & 0xFF),
		  (latency & 0xFF00) >> 8,
		  (dynamicRange & 0xFF),
		  (dynamicRange & 0xFF00) >> 8,
		  (sensitivity & 0xFF),
		  (sensitivity & 0xFF00) >> 8};
		this->write(util::to_span(sensorConfig));
		std::uint8_t rxData[8];
		this->read(Register::Parameter_Write_Buffer, util::total_size(rxData), rxData);

		static constexpr util::byte parameterRequest[] = {Register::Parameter_Request, 0x80 | sensorID};
		this->write(util::to_span(parameterRequest));

		// Print out parameters
		fmt::print("[BHI160:] Config Parameter Data: \n");
		for(auto const& e : rxData) fmt::print("{};", e);
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
			_state = State::Idle;
		}
		this->read(Register::Buffer_out, bytesToSend, rxData);
		std::span printSpan(rxData, rxData + _bytesInFIFO);
		//fmt::print("BHI160: Buffer data: {:#4x}\n", fmt::join(std::as_bytes(printSpan), ", "));

		static constexpr util::byte flushSensorPackage[] = {Register::FIFO_Flush, 35};
		this->write(util::to_span(flushSensorPackage));

		HandleData(printSpan);
	}

	void BHI160::PrintVersionAndStatus()
	{
		std::uint16_t RAMVersion;
		this->read(Register::RAM_Version, sizeof(RAMVersion), reinterpret_cast<util::byte*>(&RAMVersion));
		fmt::print("[BHI160:] RAM Version 0x{:02x}\n", RAMVersion);

		std::uint16_t ROMVersion;
		this->read(Register::ROM_Version, sizeof(ROMVersion), reinterpret_cast<util::byte*>(&ROMVersion));
		ROMVersion = util::byte_swap(ROMVersion);
		fmt::print("[BHI160:] ROM Version 0x{:02x}\n", ROMVersion);

		util::byte ChipStatus;
		this->read(Register::Chip_Status, 1, &ChipStatus);
		fmt::print("[BHI160:] ChipStatus: 0x{:02x}\n", ChipStatus);
	}

	void BHI160::Handler()
	{
		switch(_state)
		{
		case State::Reset:
			fmt::print("[BHI160:] Resetting...\n");
			Reset();
			_state = State::WaitForInterrupt1;
			break;

		case State::WaitForInterrupt1:
			if(gpio_get_level(config::BHI160::INTERRUPT_PIN))
			{
				StartRAMPatch();
				_state = State::FirmwareUpload;
				fmt::print("[BHI160:] Uploading firmware...\n");
			}
			break;

		case State::FirmwareUpload:
			UploadFirmware();
			_state = State::ModeSwitch;
			break;

		case State::ModeSwitch:
			StartCPU();

			_state = State::WaitForInterrupt2;
			break;

		case State::WaitForInterrupt2:
			if(gpio_get_level(config::BHI160::INTERRUPT_PIN))
			{
				fmt::print("[BHI160:] Configuring device...\n");
				PrintVersionAndStatus();

				_state = State::Configuration;
			}
			break;

		case State::Configuration:
			ConfigureDevices();
			_state = State::Idle;
			break;

		case State::Idle:
			if(gpio_get_level(config::BHI160::INTERRUPT_PIN) == 0) 
				break;

			GetRemainingFIFOSize();

			if(_bytesInFIFO == 0)
				break;
			_state = State::GetData;
			nobreak;

		case State::GetData:
			GetData();
			break;
		}
	}

	bool BHI160::IsReady() const
	{
		return _state >= State::Idle;
	}

	mem::ring_buffer_t* BHI160::RingBuffer()
	{
		if(!_buffer.buffer)
		{
			fmt::print("[BHI160:] Ring buffer was not initialized!\n");
		}
		return &_buffer;
	}
}
