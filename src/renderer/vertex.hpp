#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace ps::renderer {

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};

}  // namespace ps::renderer
