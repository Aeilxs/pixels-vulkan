#pragma once

#include "gfx/particles/particle.hpp"
#include "image/image.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace ps::gfx::particles {

/// @brief Owns the particles generated from an image and exposes them as a contiguous view.
class ParticleSystem {
   public:
    /// @brief Builds particles by sampling non-transparent image pixels at the requested gap.
    /// @param image Source RGBA image.
    /// @param gap Sampling step in pixels.
    /// @return A particle system whose positions are expressed in source-image coordinates.
    /// @throws std::invalid_argument If @p gap is zero.
    static ParticleSystem fromImage(const ps::image::Image& image, std::uint32_t gap);

    /// @brief Returns a non-owning contiguous view of the current particles.
    /// @return A span valid until the particle storage is modified or this object is destroyed.
    [[nodiscard("ParticleSystem::particles() returns a span of particles. If you don't use it, you might be doing something wrong.")]]
    std::span<const Particle> particles() const noexcept;

   private:
    std::vector<Particle> particles_;
};

}  // namespace ps::gfx::particles
