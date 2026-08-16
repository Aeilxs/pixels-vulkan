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
    /// @brief Creates a command pool for the selected graphics queue family.
    /// @param physicalDevice Physical device providing the graphics queue family.
    /// @param device Logical device that owns the command pool.
    /// @throws std::runtime_error If Vulkan cannot create the command pool.
    CommandPool(const PhysicalDevice& physicalDevice, const Device& device);

    /// @brief Destroys the owned Vulkan command pool.
    ~CommandPool();

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    CommandPool(CommandPool&&) = delete;
    CommandPool& operator=(CommandPool&&) = delete;

    /// @brief Returns the native Vulkan command-pool handle without transferring ownership.
    /// @return The owned command-pool handle, valid for this object's lifetime.
    [[nodiscard("The Vulkan command pool handle must be used")]]
    VkCommandPool nativeHandle() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkCommandPool handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
