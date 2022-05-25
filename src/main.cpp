#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_util/spiHost.hpp"
#include "display/display.hpp"

#include <fmt/format.h>


extern "C" void app_main() {
    fmt::print("Initializing SPI2...\n");
    esp::spiHost spi2{SPI2_HOST, GPIO_NUM_25, GPIO_NUM_23, GPIO_NUM_19, 16 * 320 * 2 + 8, SPI_DMA_CH_AUTO};
    fmt::print("Done!\n");
    fmt::print("Initializing Display...\n");
    Display display{spi2, GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_18, GPIO_NUM_5};
    //esp::spiDevice dev1{spi2, 10*1000*1000, GPIO_NUM_22, 7, 0};
    fmt::print("Done!\n");
    return;
}
