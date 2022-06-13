#include "display/display.hpp"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "esp_util/spiHost.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spiConfig.h"

#include <fmt/format.h>
#include <chrono>

extern "C" void app_main() {
    esp::spiHost spi2{
      SPI2Config::SPIHost,
      SPI2Config::MISO,
      SPI2Config::MOSI,
      SPI2Config::SCK,
      SPI2Config::transferSize,
      SPI2Config::DMAChannel};
    Display<displayConfig::CS, displayConfig::DC, displayConfig::RST, displayConfig::BCKL>
      display{spi2};

    auto now = std::chrono::system_clock::now();
    auto everySecond = now + std::chrono::seconds(1);
    while(1) {
        now = std::chrono::system_clock::now();
        if(now > everySecond){
            display.handler();
            everySecond = now + std::chrono::seconds(1);
        }
        display.flush();
    }

    return;
}
