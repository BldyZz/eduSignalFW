//
// Created by patrick on 8/14/22.
//
#pragma once
#include "esp_util/i2cDevice.hpp"
#include "fmt/format.h"

#include <array>
#include <cstdint>
#include <optional>

template<typename I2CConfig>
struct MAX30102 : esp::i2cDevice<I2CConfig, 0x57> {
    MAX30102() {}
    struct Register {
        static constexpr std::byte InterruptStatus1{0x00};
        static constexpr std::byte InterruptStatus2{0x01};
        static constexpr std::byte InterruptEnable1{0x02};
        static constexpr std::byte InterruptEnable2{0x03};
        static constexpr std::byte FiFoWrite{0x04};
        static constexpr std::byte OverflowCounter{0x05};
        static constexpr std::byte FiFoRead{0x06};
        static constexpr std::byte FiFoDataRegister{0x07};
        static constexpr std::byte FiFoConfig{0x08};
        static constexpr std::byte ModeConfig{0x09};
        static constexpr std::byte SPO2Config{0x0A};
        static constexpr std::byte LED1PulseAmplitude{0x0C};
        static constexpr std::byte LED2PulseAmplitude{0x0D};
        static constexpr std::byte LEDMode1{0x11};
        static constexpr std::byte LEDMode2{0x12};
        static constexpr std::byte DieTempInteger{0x1F};
        static constexpr std::byte DieTempFraction{0x20};
        static constexpr std::byte DieTempConfig{0x21};
        static constexpr std::byte RevisionID{0xFE};
        static constexpr std::byte PartID{0xFF};
    };

    struct Command {
        static constexpr std::array Reset{Register::ModeConfig, std::byte{0b0100'0000}};
    };

    std::vector<std::pair<std::uint32_t, std::uint32_t>> dataBuffer;

    enum class State { reset, init, idle, readData, changeConfig };
    State        st{State::reset};
    int numberOfSamples{};

    void         handler() {
                switch(st) {
                case State::reset:
            {
                        fmt::print("MAX30102: Resetting...\n");
                        this->write(Command::Reset);
                        st = State::init;
            }
            break;

                case State::init:
            {
                        fmt::print("MAX30102: Initialization...\n");
                        static constexpr std::byte modeControl{0b011};
                        this->write(std::array{Register::ModeConfig, std::byte{modeControl}});
                        static constexpr std::byte sampleRate{0b001};
                        static constexpr std::byte pulseWidth{0b11};
                        static constexpr std::byte adcRange{0b11};
                        static constexpr std::byte spo2ConfigData{
                  adcRange << 5 | sampleRate << 2 | pulseWidth};
                        this->write(std::array{Register::SPO2Config, spo2ConfigData});
                        static constexpr std::byte sampleAverage{0b001};
                        static constexpr std::byte fifoRolloverEnable{0b0};
                        static constexpr std::byte fifoAFull{0b0000};
                        static constexpr std::byte fifoConfigData{
                  sampleAverage << 5 | fifoRolloverEnable << 4 | fifoAFull};
                        this->write(std::array{Register::FiFoConfig, fifoConfigData});
                        static constexpr std::byte ledBrightness{128};
                        this->write(std::array{Register::LED1PulseAmplitude, ledBrightness});
                        this->write(std::array{Register::LED2PulseAmplitude, ledBrightness});
                        st = State::idle;
            }
            break;

                case State::idle:
            {
                        //TODO: Get Read and Write Pointer and calculate available Samples
                std::uint8_t ReadPointer{};
                this->read(Register::FiFoRead, 1, &ReadPointer);
                std::uint8_t WritePointer{};
                this->read(Register::FiFoWrite, 1, &WritePointer);
                if(ReadPointer != WritePointer) {
                    numberOfSamples = WritePointer - ReadPointer;
                    if(numberOfSamples < 0){
                        numberOfSamples += 32;
                    }
                    st = State::readData;
                }
            }
            break;

                case State::readData:
            {
                        std::array<std::uint8_t, 6> rxData{};
                        this->read(Register::FiFoDataRegister, rxData.size(), rxData.data());
                        std::ranges::reverse(rxData);

                        std::uint32_t tempRed{0};
                        std::memcpy(&tempRed, &rxData[3], 3);
                        tempRed &= 0x3FFFF;

                        std::uint32_t tempInfraRed{0};
                        std::memcpy(&tempInfraRed, &rxData[0], 3);
                        tempInfraRed &= 0x3FFFF;

                        if(dataBuffer.size() < 100)
                        {
                            dataBuffer.push_back(std::make_pair(tempRed,tempInfraRed));
                        }
                        else{
                            fmt::print("MAX30102: ERROR: To much samples in buffer!\n");
                        }
                        --numberOfSamples;
                        st       = State::changeConfig;
            }
            break;

                case State::changeConfig:
            {
                        //TODO: Update configuration of the sensor while measuring...
                        if(numberOfSamples > 0){
                            st = State::readData;
                        }
                        else {
                            st = State::idle;
                        }
            }
            break;
        }
    }
};