//
// Created by patrick on 5/29/22.
//
#pragma once
#include "display/displayConfig.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"

namespace SPI2Config {
    static constexpr auto MISO{GPIO_NUM_25};
    static constexpr auto MOSI{GPIO_NUM_23};
    static constexpr auto SCK{GPIO_NUM_19};
    static constexpr auto SPIHost{SPI2_HOST};
    static constexpr auto transferSize{
            displayConfig::parallelSend * displayConfig::displayWidth * 2 + 8};
    static constexpr auto DMAChannel{SPI_DMA_CH_AUTO};
} //namespace SPI2Config