#pragma once

#include "gfx/particles/particle.hpp"
#include "image/image.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace ps::gfx::particles {

class ParticleSystem {
   public:
    static ParticleSystem fromImage(const ps::image::Image& image, std::uint32_t gap);

    [[nodiscard("ParticleSystem::particles() returns a span of particles. If you don't use it, you might be doing something wrong.")]]
    std::span<const Particle> particles() const;

   private:
    std::vector<Particle> particles_;
};

}  // namespace ps::gfx::particles
