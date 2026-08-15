#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace ps::image {

/// @brief One 8-bit RGBA pixel.
struct Pixel {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{};
};

static_assert(sizeof(Pixel) == 4, "Pixel struct must be 4 bytes in size");

/// @brief Decoded image stored as a tightly packed row-major RGBA pixel array.
struct Image {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<Pixel> pixels{};
};

/// @brief Decodes an image file into 8-bit RGBA pixels.
/// @param path Path to the image file to decode.
/// @return The decoded image data.
/// @throws std::runtime_error If decoding fails or the decoded dimensions are invalid.
Image load(const std::filesystem::path& path);

}  // namespace ps::image
