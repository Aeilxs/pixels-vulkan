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
    /// @brief Creates a Vulkan surface for an SDL window.
    /// @param instance Vulkan instance used to create and later destroy the surface.
    /// @param window SDL window associated with the presentation surface.
    /// @pre Both @p instance and @p window must outlive this surface.
    /// @throws std::runtime_error If SDL cannot create the Vulkan surface.
    Surface(const Instance& instance, const ps::platform::Window& window);

    /// @brief Destroys the owned Vulkan surface.
    ~Surface();

    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(Surface&&) = delete;
    Surface& operator=(Surface&&) = delete;

    /// @brief Returns the native Vulkan surface handle without transferring ownership.
    /// @return The owned Vulkan surface handle, valid for this object's lifetime.
    [[nodiscard("The Vulkan surface handle must be used")]]
    VkSurfaceKHR nativeHandle() const;

   private:
    VkInstance instance_{VK_NULL_HANDLE};
    VkSurfaceKHR handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
