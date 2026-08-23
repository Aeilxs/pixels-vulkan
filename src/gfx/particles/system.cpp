#include "gfx/colors/colors.hpp"
#include "gfx/particles/system.hpp"

#include <cmath>
#include <cstddef>
#include <random>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/norm.hpp>
#include <stdexcept>

namespace image = ps::image;

namespace ps::gfx::particles {
ParticleSystem ParticleSystem::fromImage(const image::Image& image, std::uint32_t gap) {
    if (gap == 0) {
        throw std::invalid_argument("Gap must be greater than zero.");
    }

    ParticleSystem system{};
    system.imageDimensions_.width = image.width;
    system.imageDimensions_.height = image.height;

    const std::size_t rows = (static_cast<std::size_t>(image.height) + gap - 1) / gap;
    const std::size_t cols = (static_cast<std::size_t>(image.width) + gap - 1) / gap;
    const std::size_t capacity = rows * cols;

    system.positions_.reserve(capacity);
    system.origins_.reserve(capacity);
    system.velocities_.reserve(capacity);
    system.colors_.reserve(capacity);

    for (std::uint32_t y = 0; y < image.height; y += gap) {
        for (std::uint32_t x = 0; x < image.width; x += gap) {
            // Compute the index in size_t to avoid 32-bit overflow.
            const std::size_t index = static_cast<std::size_t>(y) * image.width + x;
            const image::Pixel& pixel = image.pixels[index];

            if (pixel.a == 0) {
                continue;
            }

            const glm::vec2 position{static_cast<float>(x), static_cast<float>(y)};

            system.positions_.push_back(position);
            system.origins_.push_back(position);
            system.velocities_.emplace_back(0.0F, 0.0F);
            system.colors_.emplace_back(
                ps::gfx::colors::srgb8ToLinear(pixel.r),
                ps::gfx::colors::srgb8ToLinear(pixel.g),
                ps::gfx::colors::srgb8ToLinear(pixel.b),
                static_cast<float>(pixel.a) / 255.0F
            );
        }
    }

    if (system.positions_.empty()) {
        throw std::runtime_error{"No particles generated from image; check the gap and image alpha channel."};
    }

    return system;
}

void ParticleSystem::randomize() {
    std::uniform_real_distribution<float> xDistribution{0.0F, static_cast<float>(imageDimensions_.width)};
    std::uniform_real_distribution<float> yDistribution{0.0F, static_cast<float>(imageDimensions_.height)};

    for (std::size_t i = 0; i < positions_.size(); ++i) {
        positions_[i].x = xDistribution(randomEngine_);
        positions_[i].y = yDistribution(randomEngine_);
        velocities_[i] = {};
    }
}

void ParticleSystem::update(float dt, const std::optional<glm::vec2>& mousePosition, Effect effects) {
    constexpr float damping = 3.0F;
    constexpr float gravityAcceleration = 1200.0F;

    const float dampingFactor = std::exp(-damping * dt);
    const bool springEnabled = hasEffect(effects, Effect::Spring);
    const bool repulsionEnabled = hasEffect(effects, Effect::Repulsion);
    const bool attractionEnabled = hasEffect(effects, Effect::Attraction);
    const bool vortexEnabled = hasEffect(effects, Effect::Vortex);
    const bool gravityEnabled = hasEffect(effects, Effect::Gravity);

    for (std::size_t i = 0; i < positions_.size(); ++i) {
        glm::vec2& position = positions_[i];
        glm::vec2& velocity = velocities_[i];
        const glm::vec2& origin = origins_[i];

        glm::vec2 acceleration{0.0F};

        if (springEnabled) {
            acceleration += springAcceleration(position, origin);
        }

        if (mousePosition.has_value()) {
            if (repulsionEnabled) {
                acceleration += repulsionAcceleration(position, *mousePosition);
            }
            if (attractionEnabled) {
                acceleration += attractionAcceleration(position, *mousePosition);
            }
            if (vortexEnabled) {
                acceleration += vortexAcceleration(position, *mousePosition);
            }
        }

        if (gravityEnabled) {
            acceleration.y += gravityAcceleration;
        }

        velocity += acceleration * dt;
        velocity *= dampingFactor;
        position += velocity * dt;
    }
}

glm::vec2 ParticleSystem::springAcceleration(const glm::vec2& position, const glm::vec2& origin) noexcept {
    constexpr float stiffness = 30.0F;
    return (origin - position) * stiffness;
}

glm::vec2 ParticleSystem::repulsionAcceleration(const glm::vec2& position, const glm::vec2& mousePosition) noexcept {
    constexpr float radius = 800.0F;
    constexpr float strength = 8000.0F;
    constexpr float minDistance2 = 0.0001F;

    const glm::vec2 delta = position - mousePosition;
    const float distance2 = glm::length2(delta);
    const float radius2 = radius * radius;

    if (distance2 >= radius2 || distance2 <= minDistance2) {
        return {};
    }

    const float distance = std::sqrt(distance2);
    const glm::vec2 direction = delta / distance;
    const float falloff = 1.0F - distance / radius;
    return direction * strength * falloff;
}

glm::vec2 ParticleSystem::attractionAcceleration(const glm::vec2& position, const glm::vec2& mousePosition) noexcept {
    constexpr float radius = 1200.0F;
    constexpr float strength = 5000.0F;
    constexpr float minDistance2 = 0.0001F;

    const glm::vec2 delta = mousePosition - position;
    const float distance2 = glm::length2(delta);
    const float radius2 = radius * radius;

    if (distance2 >= radius2 || distance2 <= minDistance2) {
        return {};
    }

    const float distance = std::sqrt(distance2);
    const glm::vec2 direction = delta / distance;
    const float falloff = 1.0F - distance / radius;
    return direction * strength * falloff;
}

glm::vec2 ParticleSystem::vortexAcceleration(const glm::vec2& position, const glm::vec2& mousePosition) noexcept {
    constexpr float radius = 1400.0F;
    constexpr float strength = 7000.0F;
    constexpr float minDistance2 = 0.0001F;

    const glm::vec2 delta = position - mousePosition;
    const float distance2 = glm::length2(delta);
    const float radius2 = radius * radius;

    if (distance2 >= radius2 || distance2 <= minDistance2) {
        return {};
    }

    const float distance = std::sqrt(distance2);
    const glm::vec2 radialDirection = delta / distance;
    const glm::vec2 tangent{-radialDirection.y, radialDirection.x};
    const float falloff = 1.0F - distance / radius;
    return tangent * strength * falloff;
}

std::span<const glm::vec2> ParticleSystem::positions() const noexcept {
    return positions_;
}

std::span<const glm::vec4> ParticleSystem::colors() const noexcept {
    return colors_;
}

std::size_t ParticleSystem::size() const noexcept {
    return positions_.size();
}

ImageDimensions ParticleSystem::imageDimensions() const noexcept {
    return imageDimensions_;
}

}  // namespace ps::gfx::particles
