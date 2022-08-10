//
// Created by patrick on 8/5/22.
//

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



template<typename SPIConfig, gpio_num_t CSPin>
struct MCP3561 : private esp::spiDevice<SPIConfig, 20> {
    explicit MCP3561(esp::spiHost<SPIConfig> const& bus)
            : esp::spiDevice<SPIConfig, 20>(bus, 10 * 1000 * 1000, CSPin, 0) {
        fmt::print("Initializing MCP3561...\n");
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


    struct Command {
        static constexpr std::byte StartConversion{0x68};
        static constexpr std::byte Standby{0x6C};
        static constexpr std::byte Shutdown{0x70};
        static constexpr std::byte FullShutdown{0x74};
        static constexpr std::byte FullReset{0x78};
        static constexpr std::byte StaticRead(std::byte Address){
            return std::byte{0x41} ^ (Address << 2);
        };
        static constexpr std::byte IncrementalWrite(std::byte Address){
            return std::byte{0x42} ^ (Address << 2);
        };
        static constexpr std::byte IncrementalRead(std::byte Address){
            return std::byte{0x43} ^ (Address << 2);
        };
    };

    struct Register{
        static constexpr std::byte ADCDATA{0x0};
        static constexpr std::byte CONFIG0{0x1};
        static constexpr std::byte CONFIG1{0x2};
        static constexpr std::byte CONFIG2{0x3};
        static constexpr std::byte CONFIG3{0x4};
        static constexpr std::byte IRQ{0x5};
        static constexpr std::byte MUX{0x6};
        static constexpr std::byte SCAN{0x7};
        static constexpr std::byte TIMER{0x8};
        static constexpr std::byte OFFESETCAL{0x9};
        static constexpr std::byte GAINCAL{0xA};
        static constexpr std::byte LOCK{0xD};
        static constexpr std::byte CRCCFG{0xF};
    };

    void handler() {
        this->sendBlocking(std::array{std::byte{0xC0}, std::byte{0xFF}, std::byte{0xEE}});
    }
};
