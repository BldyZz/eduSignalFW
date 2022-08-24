
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
#include <thread>

extern "C" void app_main() {
    //esp::spiHost<BoardSPIConfig> boardSPI;
    //ADS1299<BoardSPIConfig, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_0> ads1299{boardSPI};

    //TODO: Move CS Pin to GPIO Expander and implement
    //MCP3561<BoardSPIConfig, GPIO_NUM_25> mcp3561{boardSPI};

    //TODO: Move CS Pin of SD-Card to GPIO Expander
    //esp::spiHost<DisplaySPIConfig> displaySPI;
    //Display<displayConfig::CS, displayConfig::DC, displayConfig::RST, displayConfig::BCKL>
    //  display{spi2};


    esp::i2cMaster<I2C0_Config>      boardI2C;
    //BHI160<I2C0_Config, GPIO_NUM_39> inertialMeasurementUnit;
    //MAX30102<I2C0_Config>            pulseOxiMeter;
    //PCF8574<I2C0_Config> ioExpander;
    TSC2003<I2C0_Config> touchScreenController;


    auto now         = std::chrono::system_clock::now();
    auto everySecond = now + std::chrono::seconds(1);
    //display.handler();
    int level{0};

    while(1) {
        now = std::chrono::system_clock::now();
        if(now > everySecond) {
            everySecond = now + std::chrono::seconds(1);
        }
        //display.flush();
        //ads1299.handler();

        //mcp3561.handler();
        //inertialMeasurementUnit.handler();
        //pulseOxiMeter.handler();
        //ioExpander.handler();
    }

    return;
}
