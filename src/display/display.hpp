#pragma once


#include "esp_util/spiDevice.hpp"
#include "esp_util/spiHost.hpp"
#include "freertos/task.h"
#include "initCommands.hpp"
#include "pixel.hpp"
#include "displayBuffer.hpp"

#include <fmt/format.h>
#include <span>
#include <vector>
#include <thread>
#include <chrono>
#include "../devices/PCF8574.hpp"

template <typename Config, typename I2C_Config>
struct Display : private esp::spiDevice<typename Config::SPIConfig, Config::maxTransactions> {
    DisplayBuffer<Config::displayHeight, Config::displayWidth, Config::parallelSend> buffer;
    bool foo{false};
    bool firstFlush{true};
    PCF8574<I2C_Config>& ioExpander;
public:
    explicit Display(
      esp::spiHost<typename Config::SPIConfig> const& bus,
      PCF8574<I2C_Config>& io_expander)
      : esp::spiDevice<typename Config::SPIConfig, Config::maxTransactions>(bus, 10 * 1000 * 1000, Config::CSPin, 0), ioExpander(io_expander){
        fmt::print("Initializing Display...\n");
        gpio_set_direction(Config::DCPin, GPIO_MODE_OUTPUT);
        reset();
    }

    void queueData(std::span<std::byte const> package) {
        this->sendDMA(package, [](){
            gpio_set_level(Config::DCPin, 1);});
    }

    void queueCommand(std::byte const& data) {
        this->sendDMA(std::span{&data, 1}, [](){
            gpio_set_level(Config::DCPin, 0);});
    }

    void reset() {
        fmt::print("Resetting Display...\n");
        ioExpander.currentOutput.bit2 = 0; //Backlight
        ioExpander.currentOutput.bit1 = 0; //Reset
        ioExpander.handler();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ioExpander.currentOutput.bit1 = 1; //Reset
        ioExpander.handler();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        sendConfig();
        ioExpander.currentOutput.bit2 = 1; //Backlight
        ioExpander.handler();
    }

    void sendConfig() {
        for(auto c : lcdInitCommmands) {
            queueCommand(c.cmd);
            queueData(c.data);
            this->waitDMA(2);
            if(c.waitDelay) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        ioExpander.currentOutput.bit2 = 1; //Backlight
        ioExpander.handler();
        fmt::print("Display is fully configured!\n");
    }

    void queueLine(unsigned int const yPos, std::span<std::byte const> lineData) {
        queueCommand(std::byte{0x2A});
        queueData(std::array{
          std::byte{0},
          std::byte{0},
          std::byte((Config::displayWidth) >> 8),
          std::byte((Config::displayWidth) bitand 0xff)});
        queueCommand(std::byte{0x2B});
        queueData(std::array{
          std::byte(yPos >> 8),
          std::byte(yPos bitand 0xff),
          std::byte((yPos + Config::parallelSend) >> 8),
          std::byte((yPos + Config::parallelSend) bitand 0xff)});
        queueCommand(std::byte{0x2C});
        queueData(lineData);
    }

    //TODO: Function is currently blocking... do not wait for DMA!
    void flush() {
        if(!firstFlush){
            this->waitDMA(6);
        }
        firstFlush = false;
        static std::size_t cycle{};
        static std::size_t line{};
        if(cycle > buffer.size() - 1){
            cycle = 0;
            line = 0;
        }
        auto& lineBuffer = buffer[cycle];
        queueLine(line, std::as_bytes(std::span{lineBuffer}));
        line += Config::parallelSend;
        ++cycle;
    }

    void handler() {
        //flush();
    }
};