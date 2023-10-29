cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(EXTRA_COMPONENT_DIRS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(eduSignal)


execute_process(COMMAND git submodule update --init -- external/fmt
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_subdirectory(external/fmt)
include_directories(external/firmware)
add_subdirectory(esp_util)

target_link_libraries(${CMAKE_PROJECT_NAME}.elf PRIVATE fmt::fmt-header-only esp_util::util)

target_sources(${CMAKE_PROJECT_NAME}.elf PUBLIC
    src/main.cpp
    src/devices/ADS1299.cpp
    src/devices/BHI160.cpp
    src/devices/MAX30102.cpp
    src/devices/MCP3561.cpp
    src/devices/PCF8574.cpp
    src/devices/TSC2003.cpp
    src/tasks/sensor_control.cpp
    src/tasks/telemetry_transmitter.cpp
    src/network/wifi.cpp
    src/network/sockets.cpp
)

