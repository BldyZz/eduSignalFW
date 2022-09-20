//
// Created by patrick on 5/26/22.
//

#pragma once
#include <fmt/format.h>
#include <bitset>
#include <cstddef>

struct Pixel{
private:
    std::uint16_t pixelData;
public:
    void set(std::uint8_t const red, std::uint8_t const green, std::uint8_t const blue){
        std::uint16_t R = static_cast<std::uint8_t>(std::lerp(0.0, 31.0, red/255.0));
        std::uint16_t G = static_cast<std::uint8_t>(std::lerp(0.0, 63.0, green/255.0));
        std::uint16_t B = static_cast<std::uint8_t>(std::lerp(0.0, 31.0, blue/255.0));
        std::uint8_t upper = (R << 3) bitor (G >> 3);
        std::uint8_t lower = (G << 5) bitor (B bitand 0b11111);
        pixelData = static_cast<std::uint16_t>(lower << 8 bitor upper);
    }

    void set(Pixel const& pixel){
        pixelData = pixel.get();
    }

    void set(std::uint16_t const& data){
        pixelData = data;
    }
    
    Pixel(std::uint8_t const red, std::uint8_t const green, std::uint8_t const blue){
        set(red,green,blue);
    }
    Pixel(){
        set(0,0,0);
    }
    std::uint16_t get() const{
        return pixelData;
    }
};