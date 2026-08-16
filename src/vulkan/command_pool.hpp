#pragma once

#include <vulkan/vulkan.h>

namespace ps::vulkan {

class Device;
class PhysicalDevice;

/// @brief Owns the Vulkan command pool used to allocate graphics command buffers.
///
/// The pool is associated with the graphics queue family selected by the
/// physical device. Command buffers allocated from this pool may be reset
/// individually before being recorded again.
///
/// @note The logical Vulkan device used to create this pool must outlive it.
class CommandPool final {
   public:
    CommandPool(const PhysicalDevice& physicalDevice, const Device& device);

    ~CommandPool();

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    CommandPool(CommandPool&&) = delete;
    CommandPool& operator=(CommandPool&&) = delete;

    [[nodiscard("The Vulkan command pool handle must be used")]]
    VkCommandPool nativeHandle() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkCommandPool handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
