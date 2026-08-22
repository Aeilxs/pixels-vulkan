#pragma once

#include "vulkan/queue_family_indices.hpp"

#include <vulkan/vulkan.h>

namespace ps::vulkan {

class Instance;
class Surface;

/// @brief Selects and describes a Vulkan physical device suitable for the renderer.
///
/// The selected physical device must support the configured Vulkan API version,
/// graphics and presentation queues, all required device extensions, a usable
/// swapchain and the Vulkan features required by the renderer. Vulkan owns the
/// physical-device handle; this class retains the selected handle, queue-family
/// indices and the device's fixed memory properties.
class PhysicalDevice final {
   public:
    PhysicalDevice(const Instance& instance, const Surface& surface);

    [[nodiscard("The Vulkan physical device handle must be used")]]
    VkPhysicalDevice nativeHandle() const;

    [[nodiscard("The queue family indices must be used")]]
    const QueueFamilyIndices& queueFamilies() const;

    [[nodiscard("The selected Vulkan memory type index must be used")]]
    std::uint32_t findMemoryType(std::uint32_t filter, VkMemoryPropertyFlags properties) const;

    [[nodiscard]]
    VkMemoryPropertyFlags memoryTypeProperties(std::uint32_t memoryTypeIndex) const;

   private:
    VkPhysicalDevice handle_{VK_NULL_HANDLE};
    QueueFamilyIndices queueFamilies_;
    VkPhysicalDeviceMemoryProperties memoryProperties_{};
};

}  // namespace ps::vulkan
