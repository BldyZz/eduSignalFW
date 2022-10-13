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

template <typename Config>
struct Display : private esp::spiDevice<typename Config::SPIConfig, Config::maxTransactions> {
    bool firstFlush{true};
public:
    explicit Display(
      esp::spiHost<typename Config::SPIConfig> const& bus)
      : esp::spiDevice<typename Config::SPIConfig, Config::maxTransactions>(bus, 10 * 1000 * 1000, Config::CSPin, 0){
        fmt::print("Initializing Display...\n");
        gpio_set_direction(Config::DCPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(Config::RESETPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(Config::BACKLIGHTPin, GPIO_MODE_OUTPUT);
        reset();
    }

    void queueData(std::span<std::byte const> package) {
        this->sendDMA(package, [](){
            gpio_set_level(Config::DCPin, 1);});
    }

    void queueDataCopy(std::span<std::byte const> package) {
        this->sendDMACopy(package, [](){
            gpio_set_level(Config::DCPin, 1);});
    }

    void queueCommand(std::byte const& data) {
        this->sendDMACopy(std::span{&data, 1}, [](){
            gpio_set_level(Config::DCPin, 0);});
    }

    void sendData(std::span<std::byte const> package) {
        this->sendBlocking(package, [](){
            fmt::print("Setting DC Pin...\n");
            gpio_set_level(Config::DCPin, 1);});
    }

    void sendCommand(std::byte const& data) {
        this->sendBlocking(std::span{&data, 1}, [](){
            fmt::print("Resetting DC Pin...\n");
            gpio_set_level(Config::DCPin, 0);});
    }

    void reset() {
        fmt::print("Resetting Display...\n");
        gpio_set_level(Config::BACKLIGHTPin, 0);
        gpio_set_level(Config::RESETPin, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        gpio_set_level(Config::RESETPin, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        fmt::print("Display: Sending config...\n");
        sendConfig();
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
        gpio_set_level(Config::BACKLIGHTPin, 1);
        fmt::print("Display is fully configured!\n");
    }

    void queueLine(unsigned int const yPos, std::span<std::byte const> lineData) {
        queueCommand(std::byte{0x2A});
        queueDataCopy(
                std::array{
                        std::byte{0},
                        std::byte{0},
                        std::byte((Config::displayWidth) >> 8),
                        std::byte((Config::displayWidth) bitand 0xff)}
        );
        queueCommand(std::byte{0x2B});
        queueDataCopy(
                std::array{
                        std::byte(yPos >> 8),
                        std::byte(yPos bitand 0xff),
                        std::byte((yPos + 1) >> 8),
                        std::byte((yPos + 1) bitand 0xff)}
        );
        queueCommand(std::byte{0x2C});
        queueData(lineData);
    }
    void setPixel(unsigned int const xPos, unsigned int const yPos, Pixel pixel) {
        static constexpr std::byte ColumnAddressSet{0x2A};
        static constexpr std::byte PageAddressSet{0x2B};
        static constexpr std::byte RAMWrite{0x2C};
        queueCommand(ColumnAddressSet);
        queueDataCopy(std::array{
                std::byte(xPos >> 8),
                std::byte(xPos bitand 0xff),
                std::byte(xPos >> 8),
                std::byte(xPos bitand 0xff)});
        queueCommand(PageAddressSet);
        queueDataCopy(std::array{
                std::byte(yPos >> 8),
                std::byte(yPos bitand 0xff),
                std::byte(yPos >> 8),
                std::byte(yPos bitand 0xff)});
        queueCommand(RAMWrite);
        std::array<std::byte, 2> sendArray;
        std::memcpy(&sendArray, &pixel, 2);
        queueDataCopy(sendArray);
    }

    //TODO: Function is currently blocking... do not wait for DMA!
    void flush() {
        if(firstFlush){
            //this->waitDMA(1);
            static std::array<Pixel, 320> lineBuff;
            lineBuff.fill(Pixel{255,0,0});
            for(std::size_t i{}; i < Config::displayHeight; ++i){
                queueLine(i, std::as_bytes(std::span{lineBuff}));
                this->waitDMA(6);
                fmt::print("Display: {} Line sent!\n", i);
            }
        }
        firstFlush = false;
        //setPixel(10,0,Pixel{255,0,0});

    }

    void handler() {
        //flush();
    }
};