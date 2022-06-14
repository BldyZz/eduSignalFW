#pragma once

#include "displayConfig.h"
#include "esp_util/spiDevice.hpp"
#include "esp_util/spiHost.hpp"
#include "freertos/task.h"
#include "initCommands.h"
#include "pixel.h"
#include "displayBuffer.hpp"
#include "image.hpp"

#include <fmt/format.h>
#include <span>
#include <vector>

using displayPixelFormatType = std::uint16_t;

auto constexpr numLineBuffers{displayConfig::displayHeight / displayConfig::parallelSend};
auto constexpr maxTransactions{6*numLineBuffers};

template <gpio_num_t CSPin, gpio_num_t DCPin, gpio_num_t RSTPin, gpio_num_t BACKLPin>
struct Display : private esp::spiDevice<maxTransactions> {
    DisplayBuffer<displayConfig::displayHeight, displayConfig::displayWidth, displayConfig::parallelSend> buffer;
    bool foo{false};
    bool firstFlush{true};
public:
    explicit Display(
      esp::spiHost const& bus)
      : esp::spiDevice<maxTransactions>(bus, 10 * 1000 * 1000, CSPin, 0){
        fmt::print("Initializing Display...\n");
        gpio_set_direction(DCPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(RSTPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(BACKLPin, GPIO_MODE_OUTPUT);
        reset();
    }

    void queueData(std::span<std::byte const> package) {
        sendDMA(package, [](){
            gpio_set_level(DCPin, 1);});
    }

    void queueCommand(std::byte const& data) {
        sendDMA(std::span{&data, 1}, [](){
            gpio_set_level(DCPin, 0);});
    }

    void reset() {
        fmt::print("Resetting Display...\n");
        gpio_set_level(BACKLPin, 0);
        gpio_set_level(RSTPin, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(RSTPin, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        sendConfig();
        gpio_set_level(BACKLPin, 1);
    }

    void sendConfig() {
        for(auto c : lcdInitCommmands) {
            queueCommand(c.cmd);
            queueData(c.data);
            waitDMA(2);
            if(c.waitDelay) {
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
        gpio_set_level(BACKLPin, 1);
        fmt::print("Display is fully configured!\n");
    }

    void queueLine(unsigned int const yPos, std::span<std::byte const> lineData) {
        queueCommand(std::byte{0x2A});
        queueData(std::array{
          std::byte{0},
          std::byte{0},
          std::byte((displayConfig::displayWidth) >> 8),
          std::byte((displayConfig::displayWidth) bitand 0xff)});
        queueCommand(std::byte{0x2B});
        queueData(std::array{
          std::byte(yPos >> 8),
          std::byte(yPos bitand 0xff),
          std::byte((yPos + displayConfig::parallelSend) >> 8),
          std::byte((yPos + displayConfig::parallelSend) bitand 0xff)});
        queueCommand(std::byte{0x2C});
        queueData(lineData);
    }

    //TODO: Function is currently blocking... do not wait for DMA!
    void flush() {
        if(!firstFlush){
            waitDMA(6);
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
        line += displayConfig::parallelSend;
        ++cycle;
    }

    void handler() {
        buffer.setImage(image);
        //flush();
    }
};