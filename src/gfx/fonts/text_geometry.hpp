#pragma once

#include "gfx/fonts/text_vertex.hpp"

#include <glm/vec2.hpp>
#include <string_view>
#include <vector>

namespace ps::gfx::fonts {
class FontAtlas;

/// @brief Builds two triangles per visible glyph in top-left-origin pixel coordinates.
[[nodiscard]]
std::vector<TextVertex> buildTextVertices(std::string_view text, const FontAtlas& atlas, glm::vec2 origin);

}  // namespace ps::gfx::fonts
