#include <array>
#include <vector>
#include <cassert>
#include <bit>
#include <span>

#include "pixel.h"

//TODO: Make useful functions for accessing the display buffer!
template<std::size_t DisplayHeight, std::size_t DisplayWidth, std::size_t NumLinesParallel>
struct DisplayBuffer{
    static_assert(DisplayHeight % NumLinesParallel == 0, "DisplayHeight must be multiple of NumLinesParallel!");
    static constexpr std::size_t displaySize{DisplayHeight * DisplayWidth};
    static constexpr std::size_t blockCount{DisplayHeight / NumLinesParallel};
    static constexpr std::size_t blockSize{DisplayWidth * NumLinesParallel};

    std::array<std::vector<Pixel>, blockCount> lineBuffers;

    DisplayBuffer(){
        for(auto& b : lineBuffers){
            b = std::vector<Pixel>{blockSize};
        }
    }

    void setPixel(unsigned int x, unsigned int y, Pixel const& pixel){
        assert(y < DisplayHeight);
        assert(x < DisplayWidth);
        unsigned int vectorNum{y/NumLinesParallel};
        unsigned int rowNum{y%NumLinesParallel}; 
        auto& lineBuffer = lineBuffers[vectorNum];
        lineBuffer[x + rowNum * DisplayWidth].set(pixel);
    }

    void setImage(std::span<std::uint16_t const> imageData){
        std::size_t j{};
        for(auto i{0}; i < displaySize; ++i){
            unsigned int vectorNum{i/(blockSize)};
            unsigned int pixelNum{i%(blockSize)};
            auto& lineBuffer = lineBuffers[vectorNum];
            lineBuffer[pixelNum].set(imageData[i]);
        }
    }

    std::size_t size(){
        return lineBuffers.size();
    }

    auto& operator[](std::size_t const pos){
        return lineBuffers[pos];
    }
};
