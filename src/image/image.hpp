// image.hpp
#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace ps::image {

struct Pixel {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{};
};

static_assert(sizeof(Pixel) == 4, "Pixel struct must be 4 bytes in size");

struct Image {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<Pixel> pixels{};
};

Image load(const std::filesystem::path& path);

}  // namespace ps::image