#pragma once

#include <cstdint>
#include <optional>

namespace ps::vulkan {

struct QueueFamilyIndices {
    std::optional<std::uint32_t> graphics;

    std::optional<std::uint32_t> present;

    [[nodiscard("The queue family indices must be checked for completeness")]]
    bool complete() const noexcept {
        return graphics.has_value() && present.has_value();
    }
};

}  // namespace ps::vulkan
