#include "esp_system.h"

#include <fmt/format.h>


extern "C" void app_main() {
    fmt::print("Hello World!\n");
    return;
}
