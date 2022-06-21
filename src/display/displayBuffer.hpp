#include <array>
#include <vector>
#include <cassert>
#include <bit>
#include <span>

#include "pixel.h"

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

    //TODO: Rethink type decisions
    void setImage(std::span<std::uint16_t const> imageData){
        assert(imageData.size() == displaySize);
        std::span<std::byte const> imageSpan{std::as_bytes(imageData)};
        for(auto &lineBuffer : lineBuffers){
            std::span<std::byte> bufferSpan{std::as_writable_bytes(std::span{lineBuffer})};
            std::ranges::copy_n(imageSpan.begin(), bufferSpan.size(), bufferSpan.begin());
            imageSpan = imageSpan.subspan(bufferSpan.size());
        }
    }

    //TODO: Rethink size and operator[]
    std::size_t size() const{
        return lineBuffers.size();
    }

    auto& operator[](std::size_t const pos){
        return lineBuffers[pos];
    }
};
