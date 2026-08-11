#pragma once

#include "renderer/vulkan/queue_family_indices.hpp"

#include <vulkan/vulkan.h>

namespace ps::renderer::vulkan {

class Instance;
class Surface;

/// @brief Selects and describes a Vulkan physical device suitable for the renderer.
///
/// The selected physical device must support the configured Vulkan API version,
/// graphics and presentation queues, all required device extensions, a usable
/// swapchain and the Vulkan features required by the renderer. Vulkan owns the
/// physical-device handle; this class only retains the selected handle and queue
/// family indices.
class PhysicalDevice final {
   public:
    /// @brief Selects the first physical device satisfying all renderer requirements.
    /// @param instance Vulkan instance used to enumerate physical devices.
    /// @param surface Presentation surface used to validate queue and swapchain support.
    /// @pre @p instance must outlive this object; @p surface must be valid during construction.
    /// @throws std::runtime_error If physical-device enumeration or capability
    ///         queries fail, or if no suitable physical device exists.
    PhysicalDevice(const Instance& instance, const Surface& surface);

    /// @brief Returns the selected Vulkan physical-device handle.
    /// @return A non-owning handle managed by the Vulkan instance.
    [[nodiscard("The Vulkan physical device handle must be used")]]
    VkPhysicalDevice nativeHandle() const;

    /// @brief Returns the queue family indices selected for the device.
    /// @return Complete graphics and presentation queue family indices.
    [[nodiscard("The queue family indices must be used")]]
    const QueueFamilyIndices& queueFamilies() const;

   private:
    VkPhysicalDevice handle_{VK_NULL_HANDLE};
    QueueFamilyIndices queueFamilies_;
};

}  // namespace ps::renderer::vulkan
