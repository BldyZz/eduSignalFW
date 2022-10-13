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
add_subdirectory(external/brains-witchery)
add_subdirectory(esp_util)

target_link_libraries(${CMAKE_PROJECT_NAME}.elf PRIVATE fmt::fmt-header-only esp_util::util)
target_link_libraries(${CMAKE_PROJECT_NAME}.elf PRIVATE BHI160_Firmware)
target_compile_definitions(${CMAKE_PROJECT_NAME}.elf PUBLIC LWIP_COMPAT_SOCKET_ADDR=1)

brainsWitcheryGenerate(
        BHI160_Firmware
        INPUT_FILE external/firmware/bosch_pcb_7183_di03_bmi160-7183_di03-2-1-11824.fw
        HEADER_SIZE 16
        )


target_sources(${CMAKE_PROJECT_NAME}.elf PUBLIC
    src/main.cpp
)

