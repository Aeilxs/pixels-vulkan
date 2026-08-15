#pragma once

#include "gfx/particles/particle.hpp"
#include "image/image.hpp"

#include <cstdint>
#include <vector>

namespace ps::gfx::particles {

class ParticleSystem {
   public:
    static ParticleSystem fromImage(const ps::image::Image& image, std::uint32_t gap);

   private:
    std::vector<Particle> particles_;
};

}  // namespace ps::gfx::particles
