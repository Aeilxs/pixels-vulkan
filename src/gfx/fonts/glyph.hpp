#pragma once

#include <glm/vec2.hpp>

namespace ps::gfx::fonts {

struct Glyph {
    glm::vec2 uvMin{};
    glm::vec2 uvMax{};

    glm::vec2 size{};
    glm::vec2 offset{};

    float advance{};
};

}  // namespace ps::gfx::fonts