//
// Created by patrick on 8/14/22.
//
#pragma once
#include "esp_util/i2cDevice.hpp"
#include <chrono>
#include <array>
#include <cstdint>
#include <optional>
#include "fmt/format.h"

template <typename I2CConfig>
struct PCF8574 : private esp::i2cDevice<I2CConfig, 0x20>{
    struct outputByte{
        std::uint8_t bit0 : 1, bit1 : 1, bit2 : 1, bit3 : 1, bit4 : 1, bit5 : 1, bit6 : 1, bit7 : 1;
        std::byte value() const {
            return std::byte(bit7 << 7 | bit6 << 6 | bit5 << 5 | bit4 << 4 | bit3 << 3 | bit2 << 2 | bit1 << 1 | bit0);
        }
        auto operator<=>(const outputByte&) const = default;
    };
    outputByte currentOutput;
    std::optional<outputByte> currentInput{};
private:
    outputByte oldOutput{currentOutput};
public:


    explicit PCF8574(){
        fmt::print("PCF8574: Initializing...\n");
        fmt::print("PCF8574: Done!\n");
    }

    void handler() {
        std::array<uint8_t, 1> rxData;
        this->read(std::byte{0x00}, rxData.size(), rxData.data());
        outputByte tempByte{};
        std::memcpy(&tempByte, &rxData[0], rxData.size());
        if(currentOutput != oldOutput){
            this->write(std::array{currentOutput.value()});
            oldOutput = currentOutput;
        }
        else{
            return;
        }
    }
};