//
// Created by patrick on 8/14/22.
//
#pragma once
#include "esp_util/i2cDevice.hpp"
#include "fmt/format.h"

#include <array>
#include <chrono>
#include <cstdint>

template<typename I2CConfig, gpio_num_t NIRQ>
struct TSC2003 : private esp::i2cDevice<I2CConfig, 0x48> {
    explicit TSC2003() {
        fmt::print("TSC2003: Initializing...\n");
        gpio_set_direction(NIRQ, GPIO_MODE_INPUT);
        gpio_set_pull_mode(NIRQ, GPIO_PULLUP_ONLY);
    }

    enum class State { reset, idle, setConversion, waitForResult, getResult };
    State st{State::reset};
    using tp = std::chrono::time_point<std::chrono::system_clock>;
    tp conversionDone;
    static constexpr auto conversionTime{std::chrono::microseconds(20)};

    struct Command {
        struct Measure {
            static constexpr std::byte TEMP0{0b0000'1000};
            static constexpr std::byte VBAT1{0b0001'1000};
            static constexpr std::byte IN1{0b0010'1000};
            static constexpr std::byte TEMP1{0b0100'1000};
            static constexpr std::byte VBAT2{0b0101'1000};
            static constexpr std::byte IN2{0b0110'1000};
            static constexpr std::byte XPosition{0b1100'1000};
            static constexpr std::byte YPosition{0b1101'1000};
            static constexpr std::byte Z1Position{0b1110'1000};
            static constexpr std::byte Z2Position{0b1111'1000};
        };
        struct Activate{
            static constexpr std::byte XNegDrivers{0b1000'1000};
            static constexpr std::byte YNegDrivers{0b1001'1000};
            static constexpr std::byte YPosXNegDrivers{0b1010'1000};
        };
    };

    void handler() {
        switch(st) {
            case State::reset:
            {
                st = State::setConversion;
            }
            break;

            case State::setConversion:
            {
                this->write(std::array{Command::Measure::VBAT1});
                conversionDone = std::chrono::system_clock::now() + conversionTime;
                st = State::waitForResult;
            }
                break;

            case State::waitForResult:
            {
                auto now = std::chrono::system_clock::now();
                if(now > conversionDone){
                    st = State::getResult;
                }
            }
                break;

            case State::getResult:
            {
                //TODO: Store data in optional
                std::array<std::uint8_t, 2> rxData{0};
                this->readOnly(rxData.size(), rxData.data());
                std::uint16_t tempData{};
                std::ranges::reverse(rxData);
                std::memcpy(&tempData, &rxData[0], 2);
                st = State::setConversion;
            }
                break;

        }
    }
};