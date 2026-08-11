#pragma once

#include <vulkan/vulkan.h>

namespace ps::renderer::vulkan {

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
    /// @brief Creates the logical device for the selected physical device.
    /// @param physicalDevice Selected device providing queue families, features
    ///        and extensions.
    /// @throws std::runtime_error If Vulkan cannot create the logical device.
    explicit Device(const PhysicalDevice& physicalDevice);

    /// @brief Destroys the owned Vulkan logical device.
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    /// @brief Returns the graphics queue created with the logical device.
    /// @return Non-owning queue handle managed by the logical device.
    [[nodiscard("The Vulkan graphics queue handle must be used")]]
    VkQueue graphicsQueue() const noexcept;

    /// @brief Returns the queue used to present swapchain images.
    /// @return Non-owning queue handle managed by the logical device.
    [[nodiscard("The Vulkan presentation queue handle must be used")]]
    VkQueue presentQueue() const noexcept;

    /// @brief Returns the native Vulkan logical-device handle without
    ///        transferring ownership.
    /// @return The owned logical-device handle, valid for this object's lifetime.
    [[nodiscard("The Vulkan logical device handle must be used")]]
    VkDevice nativeHandle() const noexcept;

   private:
    VkDevice handle_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};
};

}  // namespace ps::renderer::vulkan