#pragma once

#include "esp_util/spiDevice.hpp"
#include "esp_util/spiHost.hpp"
#include "freertos/task.h"
#include "pixel.h"
#include "initCommands.h"
#include "displayConfig.h"

#include <fmt/format.h>
#include <vector>

using displayPixelFormatType = std::uint16_t;


struct Display : private esp::spiDevice {
private:
    gpio_num_t DC;
    gpio_num_t RST;
    gpio_num_t BACKL;

public:
    explicit Display(
      esp::spiHost const& bus,
      gpio_num_t          CS,
      gpio_num_t          DC,
      gpio_num_t          RST,
      gpio_num_t          BACKL)
      : esp::spiDevice(bus, 10 * 1000 * 1000, CS, 7, 0)
      , DC{DC}
      , RST{RST}
      , BACKL{BACKL} {
        fmt::print("Initializing Display...\n");
        gpio_set_direction(DC, GPIO_MODE_OUTPUT);
        gpio_set_direction(RST, GPIO_MODE_OUTPUT);
        gpio_set_direction(BACKL, GPIO_MODE_OUTPUT);
        reset();
    }
    ~Display() {
    }

    void sendData(std::vector<std::byte> const& data) const {
        gpio_set_level(DC, 1);
        sendBlocking(data);
        gpio_set_level(DC, 0);
    }

    void sendCommand(std::byte const& data) const {
        gpio_set_level(DC, 0);
        std::vector<std::byte> dataVec{data};
        sendBlocking(dataVec);
    }

    void reset() {
        fmt::print("Resetting Display...\n");
        gpio_set_level(BACKL, 0);
        gpio_set_level(RST, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(RST, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        sendConfig();
        gpio_set_level(BACKL, 1);
    }

    void sendConfig() {
        for(auto c : lcdInitCommmands) {
            sendCommand(c.cmd);
            sendData(c.data);
            if(c.waitDelay) {
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
        gpio_set_level(BACKL, 1);
        fmt::print("Display is fully configured!\n");
    }

    void sendLine(unsigned int const yPos, std::vector<std::byte> const& lineData) const {
        sendCommand({std::byte{0x2A}});
        sendData(
          {std::byte{0},
           std::byte{0},
           std::byte{(displayConfig::displayWidth) >> 8},
           std::byte{(displayConfig::displayWidth) bitand 0xff}});
        sendCommand({std::byte{0x2B}});
        sendData(
          {std::byte{yPos >> 8},
           std::byte{yPos bitand 0xff},
           std::byte{(yPos + displayConfig::parallelSend) >> 8},
           std::byte{(yPos + displayConfig::parallelSend) bitand 0xff}});
        sendCommand({std::byte{0x2C}});
        sendDMA(lineData);
    }

    void flush() const {
        DRAM_ATTR static
        std::vector<std::byte> lineBuffer(
                displayConfig::displayWidth * sizeof(displayPixelFormatType) * displayConfig::parallelSend);
        for(int i = 0; i < displayConfig::parallelSend; ++i){
            for(int j = 0; j < displayConfig::displayWidth; j+=2){
                int currentByte = (i * displayConfig::displayWidth) + j;
                Pixel p(0,255,0);
                lineBuffer[currentByte] = std::byte{(p.get() bitand 0xFF00) >> 8};

                lineBuffer[currentByte + 1] = std::byte{p.get() bitand 0xFF};
                fmt::print("0x{:2X}{:2X}, ", lineBuffer[currentByte], lineBuffer[currentByte+1]);
            }
        }
        for(int i = 0; i < displayConfig::displayHeight; i += displayConfig::parallelSend) {
            sendLine(i, lineBuffer);
            waitDMA(1);
        }

    }

    void handler() {

    }
};