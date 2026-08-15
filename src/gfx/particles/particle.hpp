#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace ps::gfx::particles {

/// @brief CPU-side state of one particle in world coordinates.
struct Particle {
    /// @brief Current particle position in world coordinates.
    glm::vec2 position{};

    /// @brief Initial world-space position used as the return target.
    glm::vec2 origin{};

    /// @brief Current world-space velocity.
    glm::vec2 velocity{};

    /// @brief Normalized RGBA color sampled from the source image.
    glm::vec4 color{};
};
}  // namespace ps::gfx::particles
