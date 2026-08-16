#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace ps::gfx::particles {

// CPU-side simulation state.
struct Particle {
    glm::vec2 position{};
    glm::vec2 origin{};
    glm::vec2 velocity{};
    /// @brief Linear RGB with alpha [0.0, 1.0]
    glm::vec4 color{};
};
}  // namespace ps::gfx::particles
