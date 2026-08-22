#include "gfx/fonts/font_atlas.hpp"
#include "gfx/fonts/glyph.hpp"
#include "gfx/fonts/text_geometry.hpp"

#include <cstddef>

namespace ps::gfx::fonts {
namespace {
constexpr float tabWidthInSpaces = 4.0F;

void appendGlyph(std::vector<TextVertex>& vertices, const Glyph& glyph, float penX, float baselineY) {
    if (glyph.size.x <= 0.0F || glyph.size.y <= 0.0F) {
        return;
    }

    const float left = penX + glyph.offset.x;
    const float top = baselineY + glyph.offset.y;
    const float right = left + glyph.size.x;
    const float bottom = top + glyph.size.y;

    const float u0 = glyph.uvMin.x;
    const float v0 = glyph.uvMin.y;
    const float u1 = glyph.uvMax.x;
    const float v1 = glyph.uvMax.y;

    vertices.insert(
        vertices.end(),
        {
            TextVertex{left, top, u0, v0},
            TextVertex{right, top, u1, v0},
            TextVertex{left, bottom, u0, v1},
            TextVertex{left, bottom, u0, v1},
            TextVertex{right, top, u1, v0},
            TextVertex{right, bottom, u1, v1},
        }
    );
}

}  // namespace

std::vector<TextVertex> buildTextVertices(std::string_view text, const FontAtlas& atlas, glm::vec2 origin) {
    std::vector<TextVertex> vertices;
    vertices.reserve(text.size() * 6);

    const float lineStartX = origin.x;
    float penX = lineStartX;
    float baselineY = origin.y + atlas.ascent();

    for (const char character : text) {
        if (character == '\n') {
            penX = lineStartX;
            baselineY += atlas.lineAdvance();
            continue;
        }

        if (character == '\r') {
            continue;
        }

        if (character == '\t') {
            penX += atlas.glyph(' ').advance * tabWidthInSpaces;
            continue;
        }

        const Glyph& glyph = atlas.glyph(character);
        appendGlyph(vertices, glyph, penX, baselineY);
        penX += glyph.advance;
    }

    return vertices;
}

}  // namespace ps::gfx::fonts
