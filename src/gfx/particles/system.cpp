#include "gfx/particles/system.hpp"

#include <cmath>
#include <cstddef>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <iostream>
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

    ParticleSystem system{};
    system.particles_ = std::move(particles);
    system.imageDimensions_.width = image.width;
    system.imageDimensions_.height = image.height;

    std::cout << "Particle count: " << system.particles_.size() << " (image dimensions: " << system.imageDimensions_.width << "x"
              << system.imageDimensions_.height << ", gap: " << gap << ")\n";
    return system;
}

void ParticleSystem::randomize() {
    for (Particle& particle : particles_) {
        particle.position.x = static_cast<float>(std::rand() % imageDimensions_.width);
        particle.position.y = static_cast<float>(std::rand() % imageDimensions_.height);
    }
}

void ParticleSystem::update(float dt, const glm::vec2& mousePosition) {
    constexpr float stiffness = 20.0F;
    constexpr float damping = 3.0F;
    constexpr float inputRadius = 400.0F;
    constexpr float repulseForce = 5000.0F;

    for (Particle& p : particles_) {
        const glm::vec2 displacement = p.origin - p.position;
        p.velocity += displacement * stiffness * dt;
        p.velocity *= std::exp(-damping * dt);
        const glm::vec2 toMouse = p.position - mousePosition;

        const float distanceSquared = glm::length2(toMouse);

        const float radiusSquared = inputRadius * inputRadius;

        if (distanceSquared >= radiusSquared) {
            continue;
        }

        const float distance = std::sqrt(distanceSquared);
        const glm::vec2 repulseDir = glm::normalize(toMouse);
        p.velocity += repulseDir * repulseForce * (1.0F - distance / inputRadius) * dt;
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