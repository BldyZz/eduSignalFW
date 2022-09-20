//
// Created by patrick on 5/29/22.
//
#pragma once
#include "display/displayConfig.hpp"
#include "driver/gpio.h"
#include "driver/spi_common.h"

namespace SPI2Config {

} //namespace SPI2Config

struct DisplaySPIConfig{
    static constexpr auto MISO{GPIO_NUM_12};
    static constexpr auto MOSI{GPIO_NUM_13};
    static constexpr auto SCK{GPIO_NUM_14};
    static constexpr auto SPIHost{HSPI_HOST};
    static constexpr auto transferSize{
            displayConfig::parallelSend * displayConfig::displayWidth * 2 + 8};
    static constexpr auto DMAChannel{SPI_DMA_CH_AUTO};
};

struct BoardSPIConfig{
    static constexpr auto MISO{GPIO_NUM_19};
    static constexpr auto MOSI{GPIO_NUM_23};
    static constexpr auto SCK{GPIO_NUM_18};
    static constexpr auto SPIHost{VSPI_HOST};
    static constexpr auto transferSize{1024};
    static constexpr auto DMAChannel{0};
};