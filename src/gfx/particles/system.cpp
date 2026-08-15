#include "gfx/particles/particle.hpp"
#include "gfx/particles/system.hpp"

#include <stdexcept>
#include <utility>

namespace image = ps::image;

namespace ps::gfx::particles {
ParticleSystem ParticleSystem::fromImage(const image::Image& image, std::uint32_t gap) {
    if (gap == 0) {
        throw std::invalid_argument("Gap must be greater than zero. You don't want to be stuck in an infinite loop, do you ?");
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
    return system;
}

ImageDimensions ParticleSystem::imageDimensions() const noexcept {
    return imageDimensions_;
}

std::span<const Particle> ParticleSystem::particles() const noexcept {
    return particles_;
}

}  // namespace ps::gfx::particles