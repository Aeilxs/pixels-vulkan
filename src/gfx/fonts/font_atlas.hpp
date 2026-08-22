#pragma once

#include "gfx/fonts/glyph.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace ps::gfx::fonts {

/// @brief Rasterized printable-ASCII glyphs packed into a single 8-bit coverage atlas.
///
/// FontAtlas is CPU-side graphics data. It owns neither Vulkan resources nor stb_truetype
/// objects; the TrueType parser is only used while constructing the atlas.
class FontAtlas final {
   public:
    static constexpr int firstCodepoint = 32;
    static constexpr int lastCodepoint = 126;
    static constexpr std::size_t glyphCount = static_cast<std::size_t>(lastCodepoint - firstCodepoint + 1);

    [[nodiscard]]
    static FontAtlas fromTrueType(const std::filesystem::path& path, float pixelHeight);

    [[nodiscard]]
    const Glyph& glyph(char character) const noexcept;

    [[nodiscard]]
    std::span<const std::uint8_t> pixels() const noexcept;

    [[nodiscard]]
    std::uint32_t width() const noexcept;

    [[nodiscard]]
    std::uint32_t height() const noexcept;

    [[nodiscard]]
    float ascent() const noexcept;

    [[nodiscard]]
    float lineAdvance() const noexcept;

   private:
    FontAtlas() = default;

    std::vector<std::uint8_t> pixels_;
    std::array<Glyph, glyphCount> glyphs_{};

    std::uint32_t width_{};
    std::uint32_t height_{};
    float ascent_{};
    float lineAdvance_{};
};

}  // namespace ps::gfx::fonts
