#include "image/image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <cstring>
#include <memory>
#include <stb_image.h>
#include <stdexcept>
#include <string>

namespace ps::image {

namespace {

struct StbiDeleter {
    void operator()(stbi_uc* pixels) const {
        stbi_image_free(pixels);
    }
};

using StbiPixels = std::unique_ptr<stbi_uc, StbiDeleter>;

}  // namespace

Image load(const std::filesystem::path& path) {
    int width = 0;
    int height = 0;

    StbiPixels pixels{stbi_load(path.string().c_str(), &width, &height, nullptr, STBI_rgb_alpha)};

    if (!pixels) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error("Failed to load image '" + path.string() + "': " + (reason != nullptr ? reason : "unknown error"));
    }

    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Invalid image dimensions: " + std::to_string(width) + "x" + std::to_string(height));
    }

    Image image{};
    image.width = static_cast<std::uint32_t>(width);
    image.height = static_cast<std::uint32_t>(height);

    const std::size_t pixelCount = static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height);

    image.pixels.resize(pixelCount);

    std::memcpy(image.pixels.data(), pixels.get(), pixelCount * sizeof(Pixel));

    return image;
}

}  // namespace ps::image