//
// Created by patrick on 8/3/22.
//

#pragma once
#include "esp_util/spiDevice.hpp"
#include "esp_util/spiHost.hpp"
#include "fmt/format.h"

#include <chrono>
#include <cstddef>
#include <span>
#include <vector>
#include <thread>

template<typename SPIConfig, gpio_num_t CSPin, gpio_num_t RESETPin, gpio_num_t PDWNPin>
struct ADS1299 : private esp::spiDevice<SPIConfig, 20> {
    explicit ADS1299(esp::spiHost<SPIConfig> const& bus)
      : esp::spiDevice<SPIConfig, 20>(bus, 10 * 1000 * 1000, CSPin, 1) {
        fmt::print("ADS1299: Initializing...\n");
        gpio_set_direction(RESETPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(PDWNPin, GPIO_MODE_OUTPUT);
    }

    enum class State {
        reset,
        powerUp,
        waitForPower,
        setExternalReference,
        startConversion,
        captureNoiseData,
        setTestSignals,
        captureTestData,
        idle
    };
    State st{State::reset};
    using tp = std::chrono::time_point<std::chrono::system_clock>;
    tp timerPowerOn;
    tp timerSettleRef;

    struct Command {
        static constexpr std::byte WAKEUP{0x02};
        static constexpr std::byte STANDBY{0x04};
        static constexpr std::byte RESET{0x06};
        static constexpr std::byte START{0x08};
        static constexpr std::byte STOP{0x0A};

        static constexpr std::byte RDATAC{0x10};
        static constexpr std::byte SDATAC{0x11};
        static constexpr std::byte RDATA{0x12};

        static constexpr std::byte RREG{0x20};
        static constexpr std::byte WREG{0x40};

        static constexpr std::byte NOP{0xFF};
    };

    void handler() {
        switch(st) {
        case State::reset:
            {
                gpio_set_level(RESETPin, 0);
                gpio_set_level(PDWNPin, 0);
                fmt::print("ADS1299: Resetting...\n");
                st = State::powerUp;
            }
            break;
        case State::powerUp:
            {
                gpio_set_level(RESETPin, 1);
                gpio_set_level(PDWNPin, 1);
                timerPowerOn = std::chrono::system_clock::now() + std::chrono::microseconds(300);
                st           = State::waitForPower;
            }
            break;
        case State::waitForPower:
            {
                auto now = std::chrono::system_clock::now();
                if(now > timerPowerOn) {
                    fmt::print("ADS1299: Power up complete!\n");
                    st = State::setExternalReference;
                }
            }
            break;

        case State::setExternalReference:
            {
                this->sendBlocking(std::array{Command::RESET});
                this->sendBlocking(std::array{Command::SDATAC});
                //SEND SDATAC Command
                //Set PDB_REFBUF = 1
                //Wait for internal Reference to settle
                timerSettleRef = std::chrono::system_clock::now() + std::chrono::microseconds(300);
                st             = State::startConversion;
            }
            break;

        case State::startConversion:
            {
                //WriteCertain Registers
                //Including Input Short after Delay
                //Start Conversion over Serial
                auto now = std::chrono::system_clock::now();
                if(now > timerSettleRef) {
                    st = State::captureNoiseData;
                }
            }
            break;

        case State::captureNoiseData:
            {
                //Capture Data and Check noise
                st = State::setTestSignals;
            }
            break;

        case State::setTestSignals:
            {
                //Set Test Signals
                st = State::captureTestData;
            }
            break;

        case State::captureTestData:
            {
                //Capture Data and Test Signals
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
