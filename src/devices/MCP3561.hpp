//
// Created by patrick on 8/5/22.
//

//
// Created by patrick on 8/3/22.
//

#pragma once
#include "driver/gpio.h"
#include "esp_util/spiDevice.hpp"
#include "esp_util/spiHost.hpp"
#include "fmt/format.h"
#include "fmt/chrono.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <span>
#include <thread>
#include <vector>

template<typename SPIConfig, std::size_t channelCount, gpio_num_t CSPin, gpio_num_t IRQPin>
struct MCP3561 : private esp::spiDevice<SPIConfig, 20> {
    std::function<void(bool)> setChipSelect;

    explicit MCP3561(esp::spiHost<SPIConfig> const& bus)
      : esp::spiDevice<SPIConfig, 20>(bus, 10 * 1000 * 1000, CSPin, 0) {
        fmt::print("MCP3561: Initializing...\n");
        gpio_set_direction(IRQPin, GPIO_MODE_INPUT);
    }

    enum class State { reset, init, config, idle, captureData, shutdown};
    State st{State::reset};
    using tp = std::chrono::time_point<std::chrono::system_clock>;
    tp resetTime;
    std::size_t errorCounter{0};

    static constexpr std::byte DeviceAddress{0b01};

    struct Command {
        static constexpr std::byte StartConversion{DeviceAddress << 6 | std::byte{0b1010'00}};
        static constexpr std::byte Standby{DeviceAddress << 6 | std::byte{0b1011'00}};
        static constexpr std::byte Shutdown{DeviceAddress << 6 | std::byte{0b1100'00}};
        static constexpr std::byte FullShutdown{DeviceAddress << 6 | std::byte{0b1101'00}};
        static constexpr std::byte FullReset{DeviceAddress << 6 | std::byte{0b1110'00}};
        static constexpr std::byte StaticRead(std::byte Address) {
            return std::byte{DeviceAddress << 6 | (Address << 2) | std::byte{0b01}};
        };
        static constexpr std::byte IncrementalWrite(std::byte Address) {
            return std::byte{DeviceAddress << 6 | (Address << 2) | std::byte{0b10}};
        };
        static constexpr std::byte IncrementalRead(std::byte Address) {
            return std::byte{DeviceAddress << 6 | (Address << 2) | std::byte{0b11}};
        };
    };

    struct Register {
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
        switch(st) {
        case State::reset:
            {
                this->sendBlocking(std::array{Command::FullReset});
                resetTime = std::chrono::system_clock::now() + std::chrono::microseconds(200);
                fmt::print("MCP3561: Resetting...\n");
                st = State::init;
            }
            break;
        case State::init:
            {
                auto now{std::chrono::system_clock::now()};
                if(now > resetTime) {
                    std::array<std::byte, 2> rxData{};
                    this->sendBlocking(
                      std::array{Command::StaticRead(Register::CONFIG0), std::byte{0x00}},
                      rxData);
                    if(rxData[1] == std::byte{0xC0}) {
                        fmt::print("MCP3561: Power up complete!\n");
                        errorCounter = 0;
                        st = State::config;
                    } else {
                        fmt::print("MCP3561: Power up failed! Chip not responding! Retrying...\n");
                        ++errorCounter;
                        st = State::reset;
                    }
                }
                if(errorCounter > 10){
                    fmt::print("MCP3561: Too many errors... Shutting down!\n");
                    fmt::print("MCP3561: Check all voltages on Chip! Maybe analog or digital supply missing!\n");
                    st = State::shutdown;
                }
            }
            break;
        case State::config:
            {
                this->sendBlocking(std::array{
                  Command::IncrementalWrite(Register::CONFIG0),
                  //CONFIG0
                  std::byte{0xE3},
                  //CONFIG1
                  std::byte{0x3C},
                  //CONFIG2
                  std::byte{0xCF},
                  //CONFIG3
                  std::byte{0xD2},
                  //IRQ
                  std::byte{0x77},
                  //MUX
                  std::byte{0x08},
                  //SCAN
                  std::byte{0x00},
                  std::byte{0x00},
                  std::byte{0x01},
                  //TIMER
                  std::byte{0x00},
                  std::byte{0x00},
                  std::byte{0x00},
                  //OFFSETCAL
                  std::byte{0x00},
                  std::byte{0x51},
                  std::byte{0x6A},
                  //GAINCAL
                  std::byte{0x00},
                  std::byte{0x00},
                  std::byte{0x00}});
                fmt::print("MCP3561: Configuration complete!\n");
                st = State::idle;
            }
            break;

        case State::idle:
            {
                if(gpio_get_level(IRQPin) == 0) {
                    st = State::captureData;
                }
            }
            break;

        case State::captureData:
            {
                static constexpr auto readSize{1+3};
                std::array<std::byte, readSize> rxData{std::byte{0x00}};
                std::array<std::byte, readSize> txData{std::byte{0x00}};
                txData[0] = Command::IncrementalRead(Register::ADCDATA);
                this->sendBlocking(txData, rxData);
                std::uint32_t transformedData{};
                std::ranges::reverse(rxData);
                std::memcpy(&transformedData, &rxData[1], 3);
                st = State::idle;

            }
            break;

            case State::shutdown:
            {

            }
                break;
        }
    }
};
