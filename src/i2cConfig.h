//
// Created by patrick on 8/12/22.
//
#pragma once

#include "driver/i2c.h"

struct I2C0_Config{
    static constexpr auto SCLPin{GPIO_NUM_22};
    static constexpr auto SDAPin{GPIO_NUM_21};
    static constexpr auto PowerPin{GPIO_NUM_27};
    static constexpr auto Number{0};
    static constexpr auto Frequency{400'000};
    static constexpr auto TXBufferSize{0};
    static constexpr auto RXBufferSize{0};
    static constexpr auto TimeoutMS{1000};
    static constexpr auto SDAPullup{GPIO_PULLUP_DISABLE};
    static constexpr auto SCLPullup{GPIO_PULLUP_DISABLE};

};