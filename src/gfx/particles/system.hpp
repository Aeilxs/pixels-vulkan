#pragma once

#include "gfx/particles/particle.hpp"
#include "image/image.hpp"

#include <cstdint>
#include <random>
#include <span>
#include <vector>

namespace ps::gfx::particles {

struct ImageDimensions {
    std::uint32_t width{};
    std::uint32_t height{};
};

class ParticleSystem {
   public:
    static ParticleSystem fromImage(const ps::image::Image& image, std::uint32_t gap);

    [[nodiscard("ParticleSystem::particles() returns a span of particles. If you don't use it, you might be doing something wrong.")]]
    std::span<const Particle> particles() const noexcept;

    [[nodiscard]]
    ImageDimensions imageDimensions() const noexcept;

    void randomize();
    void update(float dt, const glm::vec2& mousePosition);

   private:
    std::vector<Particle> particles_;
    ImageDimensions imageDimensions_{};
    std::mt19937 randomEngine_{std::random_device{}()};
};

}  // namespace ps::gfx::particles
