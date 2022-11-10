//
// Created by patrick on 8/14/22.
//
#pragma once
#include "BHI160_Firmware.hpp"
#include "esp_util/i2cDevice.hpp"
#include "fmt/format.h"
#include "fmt/chrono.h"

#include <chrono>
#include <thread>
#include <optional>

template<typename I2CConfig, gpio_num_t IntPin>
struct BHI160 : private esp::i2cDevice<I2CConfig, 0x28> {
    explicit BHI160() {
        fmt::print("BHI160: Initializing...\n");
        gpio_set_direction(IntPin, GPIO_MODE_INPUT);
    }

    enum class State {
        reset,
        waitForInterrupt1,
        firmwareUpload,
        modeSwitch,
        waitForInterrupt2,
        configuration,
        idle,
        getData
    };
    State st{State::reset};
    using tp = std::chrono::time_point<std::chrono::system_clock>;
    std::uint16_t bytesInFIFO{};

    struct Command {
        static constexpr std::byte Host_Upload_Enable{0x02};
        static constexpr std::byte CPU_Run_Request{0x01};
        static constexpr std::byte FLUSH_ALL{0xFF};
    };

    struct Register {
        static constexpr std::byte Chip_Control{0x34};
        static constexpr std::byte Reset_Request{0x9B};
        static constexpr std::byte Upload_Data{0x96};
        static constexpr std::byte Upload_Address_0{0x94};
        static constexpr std::byte Upload_Address_1{0x95};
        static constexpr std::byte Upload_CRC{0x97};
        static constexpr std::byte Chip_Status{0x37};
        static constexpr std::byte Host_Status{0x35};
        static constexpr std::byte RAM_Version{0x72};
        static constexpr std::byte ROM_Version{0x70};
        static constexpr std::byte Bytes_Remaining{0x38};
        static constexpr std::byte Buffer_out{0x00};
        static constexpr std::byte FIFO_Flush{0x32};
        static constexpr std::byte Parameter_Page_Select{0x54};
        static constexpr std::byte Parameter_Read_Buffer{0x3B};
        static constexpr std::byte Parameter_Write_Buffer{0x5C};
        static constexpr std::byte Parameter_Request{0x64};
    };

    struct Event{
        static constexpr std::byte TimestampLSWWakeUp{246};
        static constexpr std::byte TimestampLSW{252};
        static constexpr std::byte Accelerometer{1};
        static constexpr std::byte AccelerometerWakeUp{33};
        static constexpr std::byte TimestampMSWWakeUp{247};
        static constexpr std::byte TimestampMSW{253};
        static constexpr std::byte MetaWakeUp{248};
        static constexpr std::byte Meta{254};
    };

    struct Accelerometer_data{
        std::optional<std::int16_t> X{};
        std::optional<std::int16_t> Y{};
        std::optional<std::int16_t> Z{};
        std::optional<std::uint8_t> status{};
    };

    std::uint32_t Timestamp{};
    Accelerometer_data accData{};

    void handleData(std::span<std::byte> package){
        if(package.empty()){
            return;
        }
        switch(package[0]){
            case Event::TimestampLSWWakeUp:
            case Event::TimestampLSW:
            {
                std::uint16_t TimestampLSW{};
                std::memcpy(&TimestampLSW, &package[1], 2);
                Timestamp = (Timestamp & 0xFF00) | TimestampLSW;
                handleData(std::span{package.begin()+3, package.end()});
            }
            break;
            case Event::TimestampMSWWakeUp:
            case Event::TimestampMSW:
            {
                std::uint16_t TimestampMSW{};
                std::memcpy(&TimestampMSW, &package[1], 2);
                Timestamp = (Timestamp & 0xFF) | (TimestampMSW << 8);
                handleData(std::span{package.begin()+3, package.end()});
            }
                break;
            case Event::Accelerometer:
            case Event::AccelerometerWakeUp:
            {
                std::int16_t tempZ(std::to_integer<std::uint8_t>(package[1]) | std::to_integer<std::uint8_t>(package[2]) << 8);
                std::int16_t tempY(std::to_integer<std::uint8_t>(package[3]) | std::to_integer<std::uint8_t>(package[4]) << 8);
                std::int16_t tempX(std::to_integer<std::uint8_t>(package[5]) | std::to_integer<std::uint8_t>(package[6]) << 8);
                std::uint8_t tempStatus{std::to_integer<std::uint8_t>(package[7])};
                accData.X = tempX;
                accData.Y = tempY;
                accData.Z = tempZ;
                accData.status = tempStatus;
                handleData(std::span{package.begin()+8, package.end()});
            }
                break;

            case Event::Meta:
            case Event::MetaWakeUp:
            {
                handleData(std::span{package.begin()+4, package.end()});
            }
                break;
            default:
                return;
        }
    }

    void handler() {
        switch(st) {
        case State::reset:
            {
                fmt::print("BHI160: Resetting...\n");
                this->write(std::array{Register::Reset_Request, std::byte{0x01}});
                st = State::waitForInterrupt1;
            }
            break;

        case State::waitForInterrupt1:
            {
                if(gpio_get_level(IntPin)) {
                    this->write(std::array{Register::Chip_Control, Command::Host_Upload_Enable});
                    st = State::firmwareUpload;
                    fmt::print("BHI160: Uploading firmware...\n");
                }
            }
            break;

        case State::firmwareUpload:
            {
                this->write(
                  std::array{Register::Upload_Address_0, std::byte{0x00}, std::byte{0x00}});
                std::size_t counter{0};

                auto iterBegin{BHI160_Firmware.begin()};
                while(iterBegin < BHI160_Firmware.end()) {
                    constexpr auto bytesToWrite{16};

                    if(iterBegin + bytesToWrite > BHI160_Firmware.end()) {
                        std::array<std::byte, bytesToWrite + 1> buffer;
                        buffer[0] = Register::Upload_Data;
                        auto bytesUntilEnd{(BHI160_Firmware.end() - iterBegin)};
                        counter = counter + bytesUntilEnd;
                        std::memcpy(&buffer[1], iterBegin, bytesUntilEnd);
                        this->write(std::span{buffer.begin(), buffer.begin() + bytesUntilEnd + 1});
                    } else {
                        counter += bytesToWrite;
                        std::array<std::byte, bytesToWrite + 1> buffer;
                        buffer[0] = Register::Upload_Data;
                        std::memcpy(&buffer[1], iterBegin, bytesToWrite);
                        this->write(buffer);
                    }
                    iterBegin += bytesToWrite;
                }

                fmt::print("BHI160: {} Bytes written...\n", counter);

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                std::array<std::uint8_t, 4> RegisterCRC;
                this->read(Register::Upload_CRC, RegisterCRC.size(), &RegisterCRC[0]);
                std::ranges::reverse(RegisterCRC);
                fmt::print(
                  "BHI160: Register CRC: 0x{:02x}, Firmware CRC: 0x{:x}\n",
                  fmt::join(RegisterCRC, ""),
                  BHI160_Firmware_CRC);

                std::array<std::uint8_t, 2> RegisterAddress;
                this->read(Register::Upload_Address_0, RegisterAddress.size(), &RegisterAddress[0]);
                //this->read(Register::Upload_Address_1, 1, &RegisterAddress[1]);
                fmt::print("BHI160: Register Address 0x{:02x}\n", fmt::join(RegisterAddress, ""));

                st = State::modeSwitch;
            }
            break;

        case State::modeSwitch:
            {
                this->write(std::array{Register::Chip_Control, Command::CPU_Run_Request});
                //Keep read after write here, otherwise Chip wont boot properly!!!
                std::uint8_t ChipControl;
                this->read(Register::Chip_Control, 1, &ChipControl);
                fmt::print("BHI160: ChipControl: 0x{:02x}\n", ChipControl);
                st = State::waitForInterrupt2;
            }
            break;

        case State::waitForInterrupt2:
            {
                if(gpio_get_level(IntPin)) {
                    fmt::print("BHI160: Configuring device...\n");

                    std::array<std::uint8_t, 2> RAMVersion;
                    this->read(Register::RAM_Version, RAMVersion.size(), &RAMVersion[0]);
                    fmt::print("BHI160: RAM Version 0x{:02x}\n", fmt::join(RAMVersion, ""));

                    std::array<std::uint8_t, 2> ROMVersion;
                    this->read(Register::ROM_Version, ROMVersion.size(), &ROMVersion[0]);
                    std::ranges::reverse(ROMVersion);
                    fmt::print("BHI160: ROM Version 0x{:02x}\n", fmt::join(ROMVersion, ""));

                    std::uint8_t ChipStatus;
                    this->read(Register::Chip_Status, 1, &ChipStatus);
                    fmt::print("BHI160: ChipStatus: 0x{:02x}\n", ChipStatus);
                    st = State::configuration;
                }
            }
            break;

        case State::configuration:
            {
                //TODO: configure sensors, meta events, fifo buffers and host interrupt
                //Disable FIFO Watermark
                //Enable Accelerometer
                //Configure Accelerometer for 50Hz and 40ms Latency
                static constexpr std::uint8_t  parameterPage{3};
                static constexpr std::uint16_t sampleRate{50};
                static constexpr std::uint16_t latency{40};
                static constexpr std::uint16_t dynamicRange{0};
                static constexpr std::uint16_t sensitivity{0};
                static constexpr std::uint8_t  sensorID{65};
                this->write(
                  std::array{Register::Parameter_Page_Select, std::byte{0b1111 & parameterPage}});

                static constexpr std::array sensorConfig{
                  Register::Parameter_Write_Buffer,
                  std::byte{(sampleRate & 0xFF)},
                  std::byte{(sampleRate & 0xFF00) >> 8},
                  std::byte{(latency & 0xFF)},
                  std::byte{(latency & 0xFF00) >> 8},
                  std::byte{(dynamicRange & 0xFF)},
                  std::byte{(dynamicRange & 0xFF00) >> 8},
                  std::byte{(sensitivity & 0xFF)},
                  std::byte{(sensitivity & 0xFF00) >> 8}};
                this->write(sensorConfig);
                std::array<std::uint8_t, 8> rxData{};
                this->read(Register::Parameter_Write_Buffer, rxData.size(), rxData.data());
                fmt::print("BHI160: Config Parameter Data: {:#4x}\n", fmt::join(rxData, ", "));
                this->write(std::array{Register::Parameter_Request, std::byte{0x80 | sensorID}});
                //Enable other Step counter with 0 latency

                st = State::idle;
            }
            break;

        case State::idle:
            {
                if(gpio_get_level(IntPin) != 0) {
                    std::array<std::uint8_t, 2> rxData;
                    this->read(Register::Bytes_Remaining, rxData.size(), rxData.data());

                    std::memcpy(&bytesInFIFO, &rxData[0], rxData.size());
                    //fmt::print("BHI160: Bytes remaining: {}\n", bytesInFIFO);
                    if(bytesInFIFO > 0) {
                        st = State::getData;
                    }
                }
            }
            break;

        case State::getData:
            {
                static constexpr auto                       maximalBufferSize{50};
                std::array<std::uint8_t, maximalBufferSize> rxData{};
                std::size_t bytesToSend{};
                if(bytesInFIFO > maximalBufferSize){
                    bytesToSend = maximalBufferSize;
                    bytesInFIFO = bytesInFIFO - maximalBufferSize;
                }
                else{
                    bytesToSend = bytesInFIFO;
                    st = State::idle;
                }
                this->read(Register::Buffer_out, bytesToSend, rxData.data());
                std::span printSpan{rxData.begin(), rxData.begin() + bytesInFIFO};
                //fmt::print("BHI160: Buffer data: {:#4x}\n", fmt::join(printSpan, ", "));
                this->write(std::array{Register::FIFO_Flush, std::byte{35}});
                handleData(as_writable_bytes(printSpan));
            }
            break;
        }
    }
};
