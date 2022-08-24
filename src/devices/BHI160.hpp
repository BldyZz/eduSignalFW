//
// Created by patrick on 8/14/22.
//
#pragma once
#include "BHI160_Firmware.hpp"
#include "esp_util/i2cDevice.hpp"
#include "fmt/format.h"

#include <chrono>
#include <thread>

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
        idle
    };
    State st{State::reset};
    using tp = std::chrono::time_point<std::chrono::system_clock>;

    struct Command {
        static constexpr std::byte Host_Upload_Enable{0x02};
        static constexpr std::byte CPU_Run_Request{0x01};
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
    };

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
                this->write(std::array{Register::Upload_Address_0, std::byte{0x00}, std::byte{0x00}});
                std::size_t counter{0};

                auto        iterBegin{BHI160_Firmware.begin()};
                while(iterBegin < BHI160_Firmware.end()) {
                    constexpr auto bytesToWrite{16};

                    if (iterBegin + bytesToWrite > BHI160_Firmware.end()) {
                        std::array < std::byte, bytesToWrite + 1 > buffer;
                        buffer[0] = Register::Upload_Data;
                        auto bytesUntilEnd{(BHI160_Firmware.end() - iterBegin)};
                        counter = counter + bytesUntilEnd;
                        std::memcpy(
                                &buffer[1],
                                iterBegin,
                                bytesUntilEnd);
                        this->write(std::span{buffer.begin(), buffer.begin() + bytesUntilEnd + 1});
                    } else {
                        counter += bytesToWrite;
                        std::array < std::byte, bytesToWrite + 1 > buffer;
                        buffer[0] = Register::Upload_Data;
                        std::memcpy(
                                &buffer[1],
                                iterBegin,
                                bytesToWrite
                        );
                        this->write(buffer);
                    }
                    iterBegin += bytesToWrite;
                }
                /*
                this->write(std::array{Register::Upload_Data});
                auto        iterBegin{BHI160_Firmware.begin()};
                while(iterBegin < BHI160_Firmware.end()) {
                    constexpr auto bytesToWrite{64};

                    if(iterBegin + bytesToWrite > BHI160_Firmware.end()) {
                        counter = counter + (BHI160_Firmware.end() - iterBegin);
                        this->write(std::span{iterBegin, BHI160_Firmware.end()});
                    } else {
                        counter += bytesToWrite;
                        this->write(std::span{iterBegin, iterBegin + bytesToWrite});
                    }
                    if(counter >= 8000)
                    {
                        break;
                    }
                    iterBegin += bytesToWrite;
                }*/




                fmt::print("{} Bytes written...\n", counter);

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                std::array<std::uint8_t, 4> RegisterCRC;
                this->read(Register::Upload_CRC, RegisterCRC.size(), &RegisterCRC[0]);
                std::ranges::reverse(RegisterCRC);
                fmt::print(
                  "Register CRC: 0x{:02x}, Firmware CRC: 0x{:x}\n",
                  fmt::join(RegisterCRC, ""),
                  BHI160_Firmware_CRC);

                std::array<std::uint8_t, 2> RegisterAddress;
                this->read(Register::Upload_Address_0, RegisterAddress.size(), &RegisterAddress[0]);
                //this->read(Register::Upload_Address_1, 1, &RegisterAddress[1]);
                fmt::print("Register Address 0x{:02x}\n", fmt::join(RegisterAddress, ""));

                st = State::modeSwitch;
            }
            break;

        case State::modeSwitch:
            {
                this->write(std::array{Register::Chip_Control, Command::CPU_Run_Request});
                std::uint8_t ChipControl;
                //Keep read after write here, otherwise Chip wont boot properly!!!
                this->read(Register::Chip_Control, 1, &ChipControl);
                fmt::print("ChipControl: 0x{:02x}\n", ChipControl);
                st = State::waitForInterrupt2;
            }
            break;

        case State::waitForInterrupt2:
            {
                if(gpio_get_level(IntPin)) {
                    st = State::configuration;
                    fmt::print("BHI160: Configuring device...\n");

                    std::array<std::uint8_t, 2> RAMVersion;
                    this->read(Register::RAM_Version, RAMVersion.size(), &RAMVersion[0]);
                    fmt::print("RAM Version 0x{:02x}\n", fmt::join(RAMVersion, ""));

                    std::array<std::uint8_t, 2> ROMVersion;
                    this->read(Register::ROM_Version, ROMVersion.size(), &ROMVersion[0]);
                    std::ranges::reverse(ROMVersion);
                    fmt::print("ROM Version 0x{:02x}\n", fmt::join(ROMVersion, ""));

                    std::uint8_t ChipStatus;
                    this->read(Register::Chip_Status, 1, &ChipStatus);
                    fmt::print("ChipStatus: 0x{:02x}\n", ChipStatus);
                }
            }
            break;

        case State::configuration:
            {
                //TODO: configure sensors, meta events, fifo buffers and host interrupt
                st = State::idle;
            }
            break;

        case State::idle:
            {
            }
            break;
        }
    }
};