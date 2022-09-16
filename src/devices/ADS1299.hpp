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
#include <thread>
#include <vector>

template<
  typename SPIConfig,
  std::size_t channelCount,
  gpio_num_t  CSPin,
  gpio_num_t  RESETPin,
  gpio_num_t  PDWNPin,
  gpio_num_t  NDRDYPin>
struct ADS1299 : private esp::spiDevice<SPIConfig, 20> {
    explicit ADS1299(esp::spiHost<SPIConfig> const& bus)
      : esp::spiDevice<SPIConfig, 20>(bus, 1 * 1000 * 1000, CSPin, 1) {
        fmt::print("ADS1299: Initializing...\n");
        gpio_set_direction(RESETPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(PDWNPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(NDRDYPin, GPIO_MODE_INPUT);
    }

    enum class State {
        reset,
        powerUp,
        waitForPower,
        setExternalReference,
        setNoiseData,
        captureNoiseData,
        setTestSignals,
        captureTestData,
        setCustomSettings,
        idle,
        captureData
    };

    State st{State::reset};
    using tp = std::chrono::time_point<std::chrono::system_clock>;
    tp                                                     timerPowerOn;
    tp                                                     timerPowerSettle;
    tp                                                     timerSettleRef;
    tp                                                     timerWaitForReset;
    tp                                                     newSampleReady;
    tp                                                     resetTime;
    std::optional<std::array<std::uint32_t, channelCount>> noiseData;
    std::optional<std::array<std::uint32_t, channelCount>> ecgData;
    std::optional<std::uint32_t>                           statusBits;

    struct Register {
        static constexpr std::byte ID{0x00};
        static constexpr std::byte CONFIG1{0x01};
        static constexpr std::byte CONFIG2{0x02};
        static constexpr std::byte CONFIG3{0x03};
        static constexpr std::byte LOFF{0x04};
        static constexpr std::byte CH1SET{0x05};
        static constexpr std::byte CH2SET{0x06};
        static constexpr std::byte CH3SET{0x07};
        static constexpr std::byte CH4SET{0x08};
        static constexpr std::byte CH5SET{0x09};
        static constexpr std::byte CH6SET{0x0A};
        static constexpr std::byte CH7SET{0x0B};
        static constexpr std::byte CH8SET{0x0C};
        static constexpr std::byte BIAS_SENSP{0x0D};
        static constexpr std::byte BIAS_SENSN{0x0E};
        static constexpr std::byte LOFF_SENSP{0x0F};
        static constexpr std::byte LOFF_SENSN{0x10};
        static constexpr std::byte LOFF_FLIP{0x11};
        static constexpr std::byte LOFF_STATP{0x12};
        static constexpr std::byte LOFF_STATN{0x13};
        static constexpr std::byte GPIO{0x14};
        static constexpr std::byte MISC1{0x15};
        static constexpr std::byte MISC2{0x16};
        static constexpr std::byte CONFIG4{0x17};
    };

    struct Command {
        static constexpr std::byte WAKEUP{0x02};
        static constexpr std::byte STANDBY{0x04};
        static constexpr std::byte RESET{0x06};
        static constexpr std::byte START{0x08};
        static constexpr std::byte STOP{0x0A};
        static constexpr std::byte RDATAC{0x10};
        static constexpr std::byte SDATAC{0x11};
        static constexpr std::byte RDATA{0x12};
        static constexpr std::byte RREG(std::byte address) {
            return std::byte{std::byte{0x20} | address};
        };
        static constexpr std::byte WREG(std::byte address) {
            return std::byte{std::byte{0x40} | address};
        };
        static constexpr std::byte BytesToWrite(std::uint8_t number) {
            return std::byte((number & 0x1F) - 1);
        }
        static constexpr std::byte NOP{0xFF};
    };

    void captureData() {
        ecgData    = {};
        statusBits = {};
        std::array<std::byte, 3 + channelCount * 3> rxData{};
        std::array<std::byte, 3 + channelCount * 3> txData{std::byte{0x00}};
        std::array<std::uint32_t, channelCount>     transformedData{};
        this->sendBlocking(txData, rxData);
        for(std::size_t i{}; i < channelCount; ++i) {
            std::memcpy(&transformedData[i], &rxData[3 + i * 3], 3);
            //transformedData[i] = transformedData[i] >> 6;
        }
        std::uint32_t tempStatusBits;
        std::memcpy(&tempStatusBits, &rxData[0], 3);
        statusBits = tempStatusBits;
        ecgData    = transformedData;
    }

    void captureNoiseData() {
        noiseData  = {};
        statusBits = {};
        std::array<std::byte, 3 + channelCount * 3> rxData{};
        std::array<std::byte, 3 + channelCount * 3> txData{std::byte{0x00}};
        std::array<std::uint32_t, channelCount>     transformedData{};
        this->sendBlocking(txData, rxData);
        for(std::size_t i{}; i < channelCount; ++i) {
            std::memcpy(&transformedData[i], &rxData[3 + i * 3], 3);
            //transformedData[i] = transformedData[i] >> 8;
        }
        std::uint32_t tempStatusBits;
        std::memcpy(&tempStatusBits, &rxData[0], 3);
        statusBits = tempStatusBits;
        noiseData  = transformedData;
    }

    void handler() {
        switch(st) {
        case State::reset:
            {
                statusBits = {};
                noiseData  = {};
                ecgData    = {};
                gpio_set_level(PDWNPin, 0);
                gpio_set_level(RESETPin, 0);
                fmt::print("ADS1299: Resetting...\n");
                timerPowerOn = std::chrono::system_clock::now() + std::chrono::microseconds(400);
                st           = State::powerUp;
            }
            break;
        case State::powerUp:
            {
                auto now = std::chrono::system_clock::now();
                if(now > timerPowerOn) {
                    gpio_set_level(PDWNPin, 1);
                    gpio_set_level(RESETPin, 1);
                    timerPowerSettle
                      = std::chrono::system_clock::now() + std::chrono::microseconds(800);
                    st = State::waitForPower;
                }
            }
            break;
        case State::waitForPower:
            {
                auto now = std::chrono::system_clock::now();
                if(now > timerPowerSettle) {
                    gpio_set_level(RESETPin, 0);
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    gpio_set_level(RESETPin, 1);
                    fmt::print("ADS1299: Power up complete!\n");
                    timerWaitForReset
                      = std::chrono::system_clock::now() + std::chrono::microseconds(300);
                    st = State::setExternalReference;
                }
            }
            break;

        case State::setExternalReference:
            {
                auto now = std::chrono::system_clock::now();
                if(now > timerWaitForReset) {
                    this->sendBlocking(std::array{Command::SDATAC});
                    std::array<std::byte, 3> rxData{};
                    this->sendBlocking(
                      std::array{
                        Command::RREG(Register::ID),
                        Command::BytesToWrite(1),
                        std::byte{0x00}},
                      rxData);
                    if((rxData[2] & std::byte{0b00011111}) == std::byte{0b00011100}) {
                        this->sendBlocking(std::array{
                          Command::WREG(Register::CONFIG3),
                          Command::BytesToWrite(1),
                          std::byte{0xE0}});
                        //Wait for internal Reference to settle
                        timerSettleRef
                          = std::chrono::system_clock::now() + std::chrono::microseconds(300);
                        fmt::print("ADS1299: Communication established!\n");
                        st = State::setNoiseData;
                    } else {
                        fmt::print("ADS1299: Could not set up device! Restarting...\n");
                        st = State::reset;
                    }
                }
            }
            break;

        case State::setNoiseData:
            {
                //Including Input Short after Delay
                //Start Conversion over Serial
                auto now = std::chrono::system_clock::now();
                if(now > timerSettleRef) {
                    //WriteCertain Registers
                    this->sendBlocking(std::array{
                      Command::WREG(Register::CONFIG1),
                      Command::BytesToWrite(1),
                      std::byte{0x96}});
                    this->sendBlocking(std::array{
                      Command::WREG(Register::CONFIG2),
                      Command::BytesToWrite(1),
                      std::byte{0xC0}});
                    //Set all channels to input Short
                    this->sendBlocking(std::array{
                      Command::WREG(Register::CH1SET),
                      Command::BytesToWrite(4),
                      std::byte{0x01},
                      std::byte{0x01},
                      std::byte{0x01},
                      std::byte{0x01}});
                    this->sendBlocking(std::array{Command::START});
                    this->sendBlocking(std::array{Command::RDATAC});
                    st = State::captureNoiseData;
                }
            }
            break;

        case State::captureNoiseData:
            {
                if(gpio_get_level(NDRDYPin) == 0) {
                    //TODO: sample more noise data and check noise
                    captureNoiseData();
                    st = State::setTestSignals;
                }
            }
            break;

        case State::setTestSignals:
            {
                //Set Test Signals
                this->sendBlocking(std::array{Command::SDATAC});
                this->sendBlocking(std::array{
                  Command::WREG(Register::CONFIG2),
                  Command::BytesToWrite(1),
                  std::byte{0xD1}});
                this->sendBlocking(std::array{
                  Command::WREG(Register::CH1SET),
                  Command::BytesToWrite(4),
                  std::byte{0x05},
                  std::byte{0x05},
                  std::byte{0x05},
                  std::byte{0x05}});
                this->sendBlocking(std::array{Command::START});
                this->sendBlocking(std::array{Command::RDATAC});
                st = State::captureTestData;
            }
            break;

        case State::captureTestData:
            {
                if(gpio_get_level(NDRDYPin) == 0) {
                    //TODO: Capture Data and Test Signals
                    captureData();
                    st = State::setCustomSettings;
                }
            }
            break;

        case State::setCustomSettings:
            {
                this->sendBlocking(std::array{Command::SDATAC});
                //Enable Lead-off Detection
                this->sendBlocking(std::array{
                  Command::WREG(Register::LOFF),
                  Command::BytesToWrite(1),
                  std::byte{0x13}});
                this->sendBlocking(std::array{
                  Command::WREG(Register::CONFIG4),
                  Command::BytesToWrite(1),
                  std::byte{0x02}});
                this->sendBlocking(std::array{
                  Command::WREG(Register::LOFF_SENSP),
                  Command::BytesToWrite(2),
                  std::byte{0xFF},
                  std::byte{0xFF}});
                //-- Setting up MUX Configuration
                this->sendBlocking(std::array{Command::SDATAC});
                //Configure Config2 Register to default value!
                this->sendBlocking(std::array{
                  Command::WREG(Register::CONFIG2),
                  Command::BytesToWrite(1),
                  std::byte{0xC0}});
                //Set all ChannelMux to Gain 1, SRB2 open,
                //Normal electrode input in normal operation mode!
                this->sendBlocking(std::array{
                  Command::WREG(Register::CH1SET),
                  Command::BytesToWrite(4),
                  std::byte{0x00},
                  std::byte{0x00},
                  std::byte{0x00},
                  std::byte{0x00}});
                this->sendBlocking(std::array{Command::START});
                this->sendBlocking(std::array{Command::RDATAC});
                fmt::print("ADS1299: Config complete!\n");
                st = State::idle;
            }
            break;

        case State::idle:
            {
                if(gpio_get_level(NDRDYPin) == 0) {
                    st = State::captureData;
                }
            }
            break;

        case State::captureData:
            {
                captureData();
                st = State::idle;
            }
            break;
        }
    }
};
