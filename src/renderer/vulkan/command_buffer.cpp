#include "renderer/vulkan/command_buffer.hpp"
#include "renderer/vulkan/command_pool.hpp"
#include "renderer/vulkan/device.hpp"

#include <stdexcept>
#include <string>

namespace ps::renderer::vulkan {

CommandBuffer::CommandBuffer(const Device& device, const CommandPool& commandPool)
    : device_{device.nativeHandle()}, commandPool_{commandPool.nativeHandle()} {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool_;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    const VkResult result = vkAllocateCommandBuffers(device_, &allocateInfo, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to allocate Vulkan command buffer: VkResult "} + std::to_string(result)};
    }
}

CommandBuffer::~CommandBuffer() {
    if (handle_ != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(device_, commandPool_, 1, &handle_);
    }
}

VkCommandBuffer CommandBuffer::nativeHandle() const noexcept {
    return handle_;
}

}  // namespace ps::renderer::vulkan
