cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(eduSignal)

execute_process(COMMAND git submodule update --init -- external/fmt
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_subdirectory(external/fmt)
add_subdirectory(esp_util)

target_link_libraries(${CMAKE_PROJECT_NAME}.elf PRIVATE fmt::fmt-header-only esp_util::util)
target_compile_definitions(${CMAKE_PROJECT_NAME}.elf PUBLIC LWIP_COMPAT_SOCKET_ADDR=1)

target_sources(${CMAKE_PROJECT_NAME}.elf PUBLIC
    src/main.cpp
)

