#pragma once

#include <cstdint>

namespace ps::gfx::particles {

enum class Effect : std::uint32_t {
    None = 0,
    Spring = 1U << 0,
    Repulsion = 1U << 1,
    Attraction = 1U << 2,
    Vortex = 1U << 3,
    Gravity = 1U << 4,
};

constexpr Effect operator|(Effect lhs, Effect rhs) noexcept {
    return static_cast<Effect>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr bool hasEffect(Effect effects, Effect effect) noexcept {
    return (static_cast<std::uint32_t>(effects) & static_cast<std::uint32_t>(effect)) != 0U;
}

}  // namespace ps::gfx::particles
