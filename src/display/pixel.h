//
// Created by patrick on 5/26/22.
//
#pragma once
#include <fmt/format.h>
#include <bitset>
#include <cstddef>

struct Pixel{
private:
    std::bitset<5> R;
    std::bitset<6> G;
    std::bitset<5> B;
public:
    void set(std::uint8_t const red, std::uint8_t const green, std::uint8_t const blue){
        auto lerp = [](float a, float b, float t){
            return a + t * (b-a);
        };
        R = static_cast<std::bitset<5>>(lerp(0.0, 31.0, red/255.0));
        G = static_cast<std::bitset<6>>(lerp(0.0, 63.0, green/255.0));
        B = static_cast<std::bitset<5>>(lerp(0.0, 31.0, blue/255.0));
    }
    Pixel(std::uint8_t const red, std::uint8_t const green, std::uint8_t const blue){
        set(red,green,blue);
    }
    Pixel(){
        set(0,0,0);
    }
};