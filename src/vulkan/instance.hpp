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
    /// @brief Creates the Vulkan instance from the shared application configuration.
    /// @throws std::runtime_error If required extensions cannot be queried or
    ///         if Vulkan instance creation fails.
    Instance();

    /// @brief Destroys the owned Vulkan instance.
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(Instance&&) = delete;
    Instance& operator=(Instance&&) = delete;

    /// @brief Returns the native Vulkan instance handle without transferring ownership.
    /// @return The owned Vulkan instance handle, valid for this object's lifetime.
    [[nodiscard("The Vulkan instance handle must be used")]]
    VkInstance nativeHandle() const;

   private:
    VkInstance handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
