

#include "display/display.hpp"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "esp_util/spiHost.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spiConfig.h"

#include <fmt/format.h>

extern "C" void app_main() {
    esp::spiHost spi2{
      SPI2Config::SPIHost,
      SPI2Config::MISO,
      SPI2Config::MOSI,
      SPI2Config::SCK,
      SPI2Config::transferSize,
      SPI2Config::DMAChannel};
    Display
      display{spi2, displayConfig::CS, displayConfig::DC, displayConfig::RST, displayConfig::BCKL};
        display.flush();
        vTaskDelay(1000/portTICK_PERIOD_MS);

    return;
}
