cmake_minimum_required(VERSION 3.1)

project(esp_util)

add_library(esp_util INTERFACE)
target_include_directories(esp_util INTERFACE src)
target_compile_features(esp_util INTERFACE cxx_std_17)

add_library(esp_util::util ALIAS esp_util)

