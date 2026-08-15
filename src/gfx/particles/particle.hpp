#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace ps::gfx::particles {

/// @brief CPU-side state of one particle in world coordinates.
struct Particle {
    glm::vec2 position{};
    glm::vec2 origin{};
    glm::vec2 velocity{};
    glm::vec4 color{};
};
}  // namespace ps::gfx::particles
