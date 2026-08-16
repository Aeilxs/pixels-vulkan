#pragma once

#include <vulkan/vulkan.h>

namespace ps::vulkan {

class PhysicalDevice;

/// @brief Owns the Vulkan logical device and exposes the required queues.
///
/// Construction enables the renderer's required Vulkan features and device
/// extensions, creates the requested queue families and retrieves the graphics
/// and presentation queue handles. The queues are owned by Vulkan as part of
/// the logical device and must not be destroyed independently.
///
/// @note The selected physical device must remain valid for the lifetime of the
/// Vulkan instance, as required by Vulkan.
class Device final {
   public:
    explicit Device(const PhysicalDevice& physicalDevice);

    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    [[nodiscard("The Vulkan graphics queue handle must be used")]]
    VkQueue graphicsQueue() const noexcept;

    [[nodiscard("The Vulkan presentation queue handle must be used")]]
    VkQueue presentQueue() const noexcept;

    [[nodiscard("The Vulkan logical device handle must be used")]]
    VkDevice nativeHandle() const noexcept;

   private:
    VkDevice handle_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
