#include "gfx/particles/system.hpp"

#include <cmath>
#include <cstddef>
#include <random>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <stdexcept>
#include <utility>

namespace image = ps::image;

namespace ps::gfx::particles {
ParticleSystem ParticleSystem::fromImage(const image::Image& image, std::uint32_t gap) {
    if (gap == 0) {
        throw std::invalid_argument("Gap must be greater than zero.");
    }

    std::vector<Particle> particles{};
    const std::size_t rows = (static_cast<std::size_t>(image.height) + gap - 1) / gap;
    const std::size_t cols = (static_cast<std::size_t>(image.width) + gap - 1) / gap;
    particles.reserve(rows * cols);

    for (std::uint32_t y = 0; y < image.height; y += gap) {
        for (std::uint32_t x = 0; x < image.width; x += gap) {
            // Compute the index in size_t to avoid 32-bit overflow.
            const std::size_t index = static_cast<std::size_t>(y) * image.width + x;
            const image::Pixel& pixel = image.pixels[index];

            if (pixel.a == 0) {
                continue;
            }

            Particle particle{};
            particle.position = glm::vec2{static_cast<float>(x), static_cast<float>(y)};
            particle.origin = particle.position;
            particle.color = glm::vec4{
                static_cast<float>(pixel.r) / 255.0f,
                static_cast<float>(pixel.g) / 255.0f,
                static_cast<float>(pixel.b) / 255.0f,
                static_cast<float>(pixel.a) / 255.0f
            };
            particles.push_back(particle);
        }
    }

    if (particles.empty()) {
        throw std::runtime_error{"No particles generated from image; check the gap and image alpha channel."};
    }

    ParticleSystem system{};
    system.imageDimensions_.width = image.width;
    system.imageDimensions_.height = image.height;
    system.particles_ = std::move(particles);
    return system;
}

void ParticleSystem::randomize() {
    std::uniform_real_distribution<float> xDistribution{0.0F, static_cast<float>(imageDimensions_.width)};
    std::uniform_real_distribution<float> yDistribution{0.0F, static_cast<float>(imageDimensions_.height)};

    for (Particle& particle : particles_) {
        particle.position.x = xDistribution(randomEngine_);
        particle.position.y = yDistribution(randomEngine_);
        particle.velocity = {};
    }
}

void ParticleSystem::update(float dt, const glm::vec2& mousePosition) {
    constexpr float stiffness = 30.0F;
    constexpr float damping = 3.0F;

    constexpr float radius = 800.0F;
    constexpr float repulsion = 8000.0F;
    constexpr float minDistance2 = 0.0001F;

    const float radius2 = radius * radius;
    const float dampingFactor = std::exp(-damping * dt);

    for (Particle& p : particles_) {
        const glm::vec2 displacement = p.origin - p.position;
        p.velocity += displacement * stiffness * dt;

        const glm::vec2 delta = p.position - mousePosition;
        const float distance2 = glm::length2(delta);

        if (distance2 < radius2 && distance2 > minDistance2) {
            const float distance = std::sqrt(distance2);
            const glm::vec2 direction = delta / distance;
            const float falloff = 1.0F - distance / radius;

            p.velocity += direction * repulsion * falloff * dt;
        }

        p.velocity *= dampingFactor;
        p.position += p.velocity * dt;
    }
}

ImageDimensions ParticleSystem::imageDimensions() const noexcept {
    return imageDimensions_;
}

std::span<const Particle> ParticleSystem::particles() const noexcept {
    return particles_;
}

}  // namespace ps::gfx::particles