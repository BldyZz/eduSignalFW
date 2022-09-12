
#include "devices/ADS1299.hpp"
#include "devices/BHI160.hpp"
#include "devices/MAX30102.hpp"
#include "devices/MCP3561.hpp"
#include "devices/PCF8574.hpp"
#include "devices/TSC2003.hpp"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_util/i2cMaster.hpp"
#include "esp_util/spiHost.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "display/display.hpp"

#include "esp_util/i2cDevice.hpp"
#include "i2cConfig.h"
#include "spiConfig.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <thread>

extern "C" void app_main() {
    //esp::spiHost<DisplaySPIConfig> displaySPI;
    //Display<displayConfig::CS, displayConfig::DC, displayConfig::RST, displayConfig::BCKL>
    //  display{spi2};

    esp::i2cMaster<I2C0_Config>      boardI2C;
    //BHI160<I2C0_Config, GPIO_NUM_39> inertialMeasurementUnit;
    //MAX30102<I2C0_Config>            pulseOxiMeter;
    //PCF8574<I2C0_Config> ioExpander;
    //TSC2003<I2C0_Config> touchScreenController;

    esp::spiHost<BoardSPIConfig> boardSPI;
    ADS1299<BoardSPIConfig, 4, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_0, GPIO_NUM_36> ecg{boardSPI};
    MCP3561<BoardSPIConfig, 8, GPIO_NUM_33, GPIO_NUM_34> adc{boardSPI};

    while(1) {
        auto now = std::chrono::system_clock::now();

        if(ecg.noiseData.has_value()){
            //fmt::print("Noise Data: {:#032b}\n",ecg.noiseData.value()[0]);
            fmt::print("Noise Data: {}\n",ecg.noiseData.value()[0]);
            ecg.noiseData = {};
        }

        if(ecg.ecgData.has_value()){
            //fmt::print("{} {:#032b}\n", std::chrono::steady_clock::now().time_since_epoch() ,ecg.ecgData.value()[0]);
            fmt::print("{} {}\n", std::chrono::steady_clock::now().time_since_epoch() ,fmt::join(ecg.ecgData.value(), ", "));
            ecg.ecgData = {};
        }

        //display.flush();
        ecg.handler();
        //adc.handler();

        //ioExpander.handler();<
        //inertialMeasurementUnit.handler();
        //pulseOxiMeter.handler();
        //display.handler();
    }

    return;
}
