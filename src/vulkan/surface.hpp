#pragma once

#include <vulkan/vulkan.h>

namespace ps::platform {
class Window;
}

namespace ps::vulkan {

class Instance;

/// @brief Owns the Vulkan presentation surface associated with an SDL window.
///
/// The surface keeps the Vulkan instance handle required for destruction, but
/// does not own either the Instance or Window objects used during construction.
/// The surface is intentionally neither copyable nor movable.
class Surface final {
   public:
    Surface(const Instance& instance, const ps::platform::Window& window);

    ~Surface();

    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(Surface&&) = delete;
    Surface& operator=(Surface&&) = delete;

    [[nodiscard("The Vulkan surface handle must be used")]]
    VkSurfaceKHR nativeHandle() const;

   private:
    VkInstance instance_{VK_NULL_HANDLE};
    VkSurfaceKHR handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
