#pragma once

#include <vulkan/vulkan.h>

namespace ps::renderer::vulkan {

class CommandPool;
class Device;

/// @brief Owns one primary Vulkan command buffer used to record graphics commands.
///
/// The command buffer is allocated from a command pool associated with the
/// graphics queue family. It is explicitly freed when this wrapper is
/// destroyed.
///
/// @note The logical Vulkan device and command pool used to allocate this
/// command buffer must both outlive it. The command buffer must not be pending
/// execution when it is destroyed.
class CommandBuffer final {
   public:
    /// @brief Allocates one primary command buffer from the supplied pool.
    /// @param device Logical device used to allocate and free the command buffer.
    /// @param commandPool Command pool from which the command buffer is allocated.
    /// @throws std::runtime_error If Vulkan cannot allocate the command buffer.
    CommandBuffer(const Device& device, const CommandPool& commandPool);

    /// @brief Frees the owned Vulkan command buffer.
    ~CommandBuffer();

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    CommandBuffer(CommandBuffer&&) = delete;
    CommandBuffer& operator=(CommandBuffer&&) = delete;

    /// @brief Returns the native Vulkan command-buffer handle without transferring ownership.
    /// @return The owned command-buffer handle, valid for this object's lifetime.
    [[nodiscard("The Vulkan command buffer handle must be used")]]
    VkCommandBuffer nativeHandle() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkCommandPool commandPool_{VK_NULL_HANDLE};
    VkCommandBuffer handle_{VK_NULL_HANDLE};
};

}  // namespace ps::renderer::vulkan
