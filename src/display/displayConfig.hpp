//
// Created by patrick on 5/29/22.
//
#pragma once
#include "driver/gpio.h"

struct displayConfig{
    static constexpr auto CSPin{GPIO_NUM_15};
    static constexpr auto DCPin{GPIO_NUM_25};

    static constexpr auto displayHeight{240};
    static constexpr auto displayWidth{320};
    static constexpr auto displaySize{displayHeight * displayWidth};
    static constexpr auto parallelSend{16};
    static constexpr auto numLineBuffers{displayHeight / parallelSend};
    static constexpr auto maxTransactions{6*numLineBuffers};

    struct SPIConfig{
        static constexpr auto MISO{GPIO_NUM_12};
        static constexpr auto MOSI{GPIO_NUM_13};
        static constexpr auto SCK{GPIO_NUM_14};
        static constexpr auto SPIHost{HSPI_HOST};
        static constexpr auto transferSize{
                displayConfig::parallelSend * displayConfig::displayWidth * 2 + 8};
        static constexpr auto DMAChannel{SPI_DMA_CH_AUTO};
        static constexpr auto Name{"Display SPI"};
    };
};