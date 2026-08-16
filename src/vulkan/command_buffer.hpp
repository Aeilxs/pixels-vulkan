#pragma once

#include <vulkan/vulkan.h>

namespace ps::vulkan {

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
    CommandBuffer(const Device& device, const CommandPool& commandPool);

    ~CommandBuffer();

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    CommandBuffer(CommandBuffer&&) = delete;
    CommandBuffer& operator=(CommandBuffer&&) = delete;

    [[nodiscard("The Vulkan command buffer handle must be used")]]
    VkCommandBuffer nativeHandle() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkCommandPool commandPool_{VK_NULL_HANDLE};
    VkCommandBuffer handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
