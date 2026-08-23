#pragma once

#include "gfx/particles/effect.hpp"
#include "image/image.hpp"

#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <optional>
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

    [[nodiscard]]
    std::span<const glm::vec2> positions() const noexcept;

    [[nodiscard]]
    std::span<const glm::vec4> colors() const noexcept;

    [[nodiscard]]
    std::size_t size() const noexcept;

    [[nodiscard]]
    ImageDimensions imageDimensions() const noexcept;

    void randomize();
    void update(float dt, const std::optional<glm::vec2>& mousePosition, Effect effects);

   private:
    std::vector<glm::vec2> positions_;
    std::vector<glm::vec2> origins_;
    std::vector<glm::vec2> velocities_;
    std::vector<glm::vec4> colors_;

    [[nodiscard]]
    static glm::vec2 springAcceleration(const glm::vec2& position, const glm::vec2& origin) noexcept;

    [[nodiscard]]
    static glm::vec2 repulsionAcceleration(const glm::vec2& position, const glm::vec2& mousePosition) noexcept;

    [[nodiscard]]
    static glm::vec2 attractionAcceleration(const glm::vec2& position, const glm::vec2& mousePosition) noexcept;

    [[nodiscard]]
    static glm::vec2 vortexAcceleration(const glm::vec2& position, const glm::vec2& mousePosition) noexcept;

    ImageDimensions imageDimensions_{};
    std::mt19937 randomEngine_{std::random_device{}()};
};

}  // namespace ps::gfx::particles
