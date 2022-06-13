#include <array>
#include <vector>
#include <cassert>

#include "pixel.h"

//TODO: Make useful functions for accessing the display buffer!
template<std::size_t DisplayHeight, std::size_t DisplayWidth, std::size_t NumLinesParallel>
struct DisplayBuffer{
    static constexpr std::size_t DisplaySize{DisplayHeight * DisplayWidth};
    static constexpr std::size_t NumLineBuffers{DisplayHeight / NumLinesParallel};
    static constexpr std::size_t lineBufferSize{DisplayWidth * NumLinesParallel};

    std::array<std::vector<Pixel>, NumLineBuffers> lineBuffers;

    DisplayBuffer(){
        for(auto& b : lineBuffers){
            b = std::vector<Pixel>{lineBufferSize};
        }
    }

    void setPixel(unsigned int x, unsigned int y, Pixel const& pixel){
        assert(y < DisplayHeight);
        assert(x < DisplayWidth);
        unsigned int arrayNum{y/NumLinesParallel};
        unsigned int rowNum{y%NumLinesParallel}; 
        auto& lineBuffer = lineBuffers[arrayNum];
        lineBuffer[x + rowNum * DisplayWidth].set(pixel);
    }

    std::size_t size(){
        return lineBuffers.size();
    }

    auto& operator[](std::size_t const pos){
        return lineBuffers[pos];
    }
};
