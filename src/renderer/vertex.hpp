#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace ps::renderer {

/// @brief Vertex format used by the current indexed-quad rendering path.
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};

}  // namespace ps::renderer
