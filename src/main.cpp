
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_util/spiHost.hpp"
#include "esp_util/i2cMaster.hpp"

#include "devices/ADS1299.hpp"
#include "devices/MCP3561.hpp"
#include "devices/BHI160.hpp"
#include "devices/MAX30102.hpp"
//#include "display/display.hpp"

#include "spiConfig.h"
#include "i2cConfig.h"

#include "esp_util/i2cDevice.hpp"

#include <fmt/format.h>
#include <chrono>
#include <thread>
#include <array>
#include <cstdint>

extern "C" void app_main() {

    //esp::spiHost<BoardSPIConfig> boardSPI;
    //ADS1299<BoardSPIConfig, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_0> ads1299{boardSPI};

    //TODO: Move CS Pin to GPIO Expander and implement
    //MCP3561<BoardSPIConfig, GPIO_NUM_25> mcp3561{boardSPI};

    //TODO: Move CS Pin of SD-Card to GPIO Expander
    //esp::spiHost<DisplaySPIConfig> displaySPI;
    //Display<displayConfig::CS, displayConfig::DC, displayConfig::RST, displayConfig::BCKL>
    //  display{spi2};

    esp::i2cMaster<I2C0_Config> boardI2C;
    //BHI160<I2C0_Config, GPIO_NUM_39> bhi160;
    MAX30102<I2C0_Config> max30102;

    auto now = std::chrono::system_clock::now();
    auto everySecond = now + std::chrono::seconds(1);
    //display.handler();
    int level{0};

    while(1) {

        now = std::chrono::system_clock::now();
        if(now > everySecond){

            everySecond = now + std::chrono::seconds(1);

        }
        //display.flush();
        //ads1299.handler();
        //bhi160.handler();
        //mcp3561.handler();
        max30102.handler();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return;
}
