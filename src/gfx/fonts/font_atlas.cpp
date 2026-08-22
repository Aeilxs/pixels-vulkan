#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "gfx/fonts/font_atlas.hpp"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ps::gfx::fonts {
namespace {
constexpr std::uint32_t atlasWidth = 512;
constexpr std::uint32_t atlasHeight = 512;
constexpr int atlasPadding = 1;

[[nodiscard]]
std::vector<std::uint8_t> readBinaryFile(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (!file.is_open()) {
        throw std::runtime_error{"Failed to open font: " + path.string()};
    }

    const std::streamsize byteSize = file.tellg();
    if (byteSize <= 0) {
        throw std::runtime_error{"Font file is empty: " + path.string()};
    }

    file.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(byteSize));
    if (!file.read(reinterpret_cast<char*>(bytes.data()), byteSize)) {
        throw std::runtime_error{"Failed to read font: " + path.string()};
    }

    return bytes;
}

}  // namespace

FontAtlas FontAtlas::fromTrueType(const std::filesystem::path& path, float pixelHeight) {
    if (pixelHeight <= 0.0F) {
        throw std::invalid_argument{"Font pixel height must be greater than zero."};
    }

    const std::vector<std::uint8_t> fontData = readBinaryFile(path);
    const int fontOffset = stbtt_GetFontOffsetForIndex(fontData.data(), 0);
    if (fontOffset < 0) {
        throw std::runtime_error{"Failed to find a TrueType font in: " + path.string()};
    }

    stbtt_fontinfo fontInfo{};
    if (stbtt_InitFont(&fontInfo, fontData.data(), fontOffset) == 0) {
        throw std::runtime_error{"Failed to initialize TrueType font: " + path.string()};
    }

    FontAtlas atlas;
    atlas.width_ = atlasWidth;
    atlas.height_ = atlasHeight;
    atlas.pixels_.resize(static_cast<std::size_t>(atlas.width_) * atlas.height_);

    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    const float metricScale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);
    atlas.ascent_ = static_cast<float>(ascent) * metricScale;
    atlas.lineAdvance_ = static_cast<float>(ascent - descent + lineGap) * metricScale;

    std::array<stbtt_packedchar, glyphCount> packedCharacters{};
    stbtt_pack_context packContext{};

    if (stbtt_PackBegin(
            &packContext,
            atlas.pixels_.data(),
            static_cast<int>(atlas.width_),
            static_cast<int>(atlas.height_),
            0,
            atlasPadding,
            nullptr
        ) == 0) {
        throw std::runtime_error{"Failed to initialize TrueType atlas packer."};
    }

    const int packed = stbtt_PackFontRange(
        &packContext,
        fontData.data(),
        0,
        pixelHeight,
        firstCodepoint,
        static_cast<int>(glyphCount),
        packedCharacters.data()
    );
    stbtt_PackEnd(&packContext);

    if (packed == 0) {
        throw std::runtime_error{"Printable ASCII glyphs did not fit in the font atlas."};
    }

    const float width = static_cast<float>(atlas.width_);
    const float height = static_cast<float>(atlas.height_);

    for (std::size_t i = 0; i < packedCharacters.size(); ++i) {
        const stbtt_packedchar& packedCharacter = packedCharacters[i];
        Glyph& glyph = atlas.glyphs_[i];

        glyph.uvMin = {
            static_cast<float>(packedCharacter.x0) / width,
            static_cast<float>(packedCharacter.y0) / height,
        };
        glyph.uvMax = {
            static_cast<float>(packedCharacter.x1) / width,
            static_cast<float>(packedCharacter.y1) / height,
        };
        glyph.size = {
            packedCharacter.xoff2 - packedCharacter.xoff,
            packedCharacter.yoff2 - packedCharacter.yoff,
        };
        glyph.offset = {
            packedCharacter.xoff,
            packedCharacter.yoff,
        };
        glyph.advance = packedCharacter.xadvance;
    }

    return atlas;
}

const Glyph& FontAtlas::glyph(char character) const noexcept {
    const int codepoint = static_cast<unsigned char>(character);
    const int safeCodepoint = (codepoint >= firstCodepoint && codepoint <= lastCodepoint) ? codepoint : '?';

    return glyphs_[static_cast<std::size_t>(safeCodepoint - firstCodepoint)];
}

std::span<const std::uint8_t> FontAtlas::pixels() const noexcept {
    return pixels_;
}

std::uint32_t FontAtlas::width() const noexcept {
    return width_;
}

std::uint32_t FontAtlas::height() const noexcept {
    return height_;
}

float FontAtlas::ascent() const noexcept {
    return ascent_;
}

float FontAtlas::lineAdvance() const noexcept {
    return lineAdvance_;
}

}  // namespace ps::gfx::fonts
