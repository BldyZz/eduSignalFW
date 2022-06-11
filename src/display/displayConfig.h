//
// Created by patrick on 5/29/22.
//
#pragma once
#include "driver/gpio.h"

namespace displayConfig{
    static constexpr auto CS{GPIO_NUM_22};
    static constexpr auto RST{GPIO_NUM_18};
    static constexpr auto DC{GPIO_NUM_21};
    static constexpr auto BCKL{GPIO_NUM_5};

    static constexpr auto displayHeight{240};
    static constexpr auto displayWidth{320};
    static constexpr auto displaySize{displayHeight * displayWidth};
    static constexpr auto parallelSend{16};
} //namespace DisplayConfig