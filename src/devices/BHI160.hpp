//
// Created by patrick on 8/14/22.
//
#pragma once
#include "esp_util/i2cDevice.hpp"
#include <chrono>
#include "fmt/format.h"

template <typename I2CConfig, gpio_num_t IntPin>
struct BHI160 : private esp::i2cDevice<I2CConfig, 0x14>{

    explicit BHI160(){
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

    };

    void handler() {
        switch(st) {
            case State::reset:
            {
                fmt::print("BHI160: Resetting...\n");
                //TODO: resetDevice
                st = State::waitForInterrupt1;
            }
                break;

            case State::waitForInterrupt1:
            {
                if(!gpio_get_level(IntPin)){
                    st = State::firmwareUpload;
                    fmt::print("BHI160: Uploading firmware...\n");
                }
            }
                break;

            case State::firmwareUpload:
            {
                st = State::modeSwitch;
            }
                break;

            case State::modeSwitch:
            {
                //TODO: switch into main execution mode
                st = State::waitForInterrupt2;
            }
                break;
            case State::waitForInterrupt2:
            {
                //TODO: wait for interrupt
                st = State::configuration;
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