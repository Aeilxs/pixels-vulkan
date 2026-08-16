#include "vulkan/command_pool.hpp"
#include "vulkan/device.hpp"
#include "vulkan/physical_device.hpp"
#include "vulkan/queue_family_indices.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace ps::vulkan {

CommandPool::CommandPool(const PhysicalDevice& physicalDevice, const Device& device) : device_{device.nativeHandle()} {
    const QueueFamilyIndices& queueFamilies = physicalDevice.queueFamilies();
    const std::uint32_t graphicsFamily = queueFamilies.graphics.value();

    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = graphicsFamily;

    const VkResult result = vkCreateCommandPool(device_, &createInfo, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan command pool: VkResult "} + std::to_string(result)};
    }
}

CommandPool::~CommandPool() {
    if (handle_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, handle_, nullptr);
    }
}

VkCommandPool CommandPool::nativeHandle() const noexcept {
    return handle_;
}

}  // namespace ps::vulkan
