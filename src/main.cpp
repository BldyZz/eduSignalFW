//#include "display/display.hpp"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "esp_util/spiHost.hpp"
#include "devices/ADS1299.hpp"
#include "devices/MCP3561.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spiConfig.h"

#include <fmt/format.h>
#include <chrono>
#include <thread>

extern "C" void app_main() {
    //esp::spiHost<DisplaySPIConfig> displaySPI;
    esp::spiHost<BoardSPIConfig> boardSPI;
    ADS1299<BoardSPIConfig, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_0> ads1299{boardSPI};
    MCP3561<BoardSPIConfig, GPIO_NUM_17> mcp3561{boardSPI};
    //Display<displayConfig::CS, displayConfig::DC, displayConfig::RST, displayConfig::BCKL>
    //  display{spi2};

    auto now = std::chrono::system_clock::now();
    auto everySecond = now + std::chrono::seconds(1);
    //display.handler();
    while(1) {

        now = std::chrono::system_clock::now();
        if(now > everySecond){

            everySecond = now + std::chrono::seconds(1);
        }
        //display.flush();
        ads1299.handler();
        mcp3561.handler();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return;
}
