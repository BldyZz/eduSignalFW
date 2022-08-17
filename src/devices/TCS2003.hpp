//
// Created by patrick on 8/14/22.
//
#pragma once
#include "esp_util/i2cDevice.hpp"
#include <chrono>
#include <array>
#include <cstdint>
#include "fmt/format.h"

//TODO: Not working properly...

template <typename I2CConfig>
struct TCS2003 : private esp::i2cDevice<I2CConfig, 0x48>{

    explicit TCS2003(){
        fmt::print("TCS2003: Initializing...\n");
    }

    enum class State {
        reset,
        idle
    };
    State st{State::reset};
    using tp = std::chrono::time_point<std::chrono::system_clock>;

    struct Command {

    };

    void handler() {
        switch(st) {


        }
    }
};