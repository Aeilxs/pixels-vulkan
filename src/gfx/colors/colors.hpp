#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace ps::gfx::colors {

inline const std::array<float, 256> srgbToLinearLut = [] {
    std::array<float, 256> lut{};
    for (std::size_t i = 0; i < lut.size(); ++i) {
        const float srgb = static_cast<float>(i) / 255.0F;
        lut[i] = srgb <= 0.04045F ? srgb / 12.92F : std::pow((srgb + 0.055F) / 1.055F, 2.4F);
    }

    return lut;
}();

[[nodiscard]]
/// @brief Converts an 8-bit sRGB color value to linear space.
inline float srgb8ToLinear(std::uint8_t k) noexcept {
    return srgbToLinearLut[k];
}

}  // namespace ps::gfx::colors