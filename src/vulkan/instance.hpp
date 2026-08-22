#pragma once

#include <vulkan/vulkan.h>

namespace ps::vulkan {

/// @brief Owns the Vulkan instance used by the renderer.
///
/// Construction enables the instance extensions required by SDL and, on
/// Apple platforms, Vulkan portability enumeration. The instance is destroyed
/// automatically and is intentionally neither copyable nor movable.
class Instance final {
   public:
    Instance();

    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(Instance&&) = delete;
    Instance& operator=(Instance&&) = delete;

    [[nodiscard("The Vulkan instance handle must be used")]]
    VkInstance nativeHandle() const;

   private:
    VkInstance handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
