#pragma once

#include <cstdint>
#include <optional>

namespace ps::vulkan {

/// @brief Queue family indices required by the renderer.
struct QueueFamilyIndices {
    /// @brief Queue family supporting graphics commands, when available.
    std::optional<std::uint32_t> graphics;

    /// @brief Queue family supporting presentation to the selected surface, when available.
    std::optional<std::uint32_t> present;

    /// @brief Reports whether every required queue family has been found.
    /// @return true when both graphics and presentation indices are available.
    [[nodiscard("The queue family indices must be checked for completeness")]]
    bool complete() const noexcept {
        return graphics.has_value() && present.has_value();
    }
};

}  // namespace ps::vulkan
