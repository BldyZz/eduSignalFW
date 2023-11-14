#***********************************************
#	Edu Signal Firmware						   *
#***********************************************
cmake_minimum_required(VERSION 3.16.0)

# C++20 with Compiler Extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Add additional projects
#set(EXTRA_COMPONENT_DIRS 
#	lcd_ui # lvgl based project for display
#)

# Include ESP-IDF. 
if(NOT DEFINED ENV{IDF_PATH})
message(FATAL_ERROR "Make sure you have installed the vscode ESP-IDF Espressif-Extension globally."
"Additionally the esp-idf environment-variable 'IDF_PATH' needs to be defined."
"For example: Under Windows 'IDF_PATH' evaluates to '%USERPROFILE%\esp\esp-idf'.")
endif()

# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
list(APPEND EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Main Project
project(eduSignal)

# Add fmt library for formatted printing
#execute_process(COMMAND git submodule update --init -- external/fmt WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
#add_subdirectory(external/fmt)
include_directories(external/firmware)

# Link to projects to executable
target_link_libraries(${CMAKE_PROJECT_NAME}.elf)
