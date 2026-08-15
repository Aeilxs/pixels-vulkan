#include "renderer/vulkan/buffer.hpp"
#include "renderer/vulkan/device.hpp"
#include "renderer/vulkan/physical_device.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

namespace ps::renderer::vulkan {
Buffer::Buffer(
    const PhysicalDevice& physicalDevice,
    const Device& device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags requiredMemoryProperties
)
    : device_{&device}, size_{size}, requiredMemoryProperties_{requiredMemoryProperties} {
    if (size_ == 0) {
        throw std::invalid_argument{"Vulkan buffer size must be greater than zero."};
    }
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size_;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device_->nativeHandle(), &bufferCreateInfo, nullptr, &buffer_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan buffer: VkResult "} + std::to_string(result)};
    }

    try {
        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(device_->nativeHandle(), buffer_, &memoryRequirements);

        const std::uint32_t memoryTypeIndex = physicalDevice.findMemoryType(memoryRequirements.memoryTypeBits, requiredMemoryProperties);

        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

        result = vkAllocateMemory(device_->nativeHandle(), &memoryAllocateInfo, nullptr, &memory_);
        if (result != VK_SUCCESS) {
            throw std::runtime_error{std::string{"Failed to allocate Vulkan memory for buffer: VkResult "} + std::to_string(result)};
        }

        result = vkBindBufferMemory(device_->nativeHandle(), buffer_, memory_, 0);
        if (result != VK_SUCCESS) {
            throw std::runtime_error{std::string{"Failed to bind Vulkan memory to buffer: VkResult "} + std::to_string(result)};
        }
    } catch (...) {
        destroy();
        throw;
    }
}

Buffer::~Buffer() {
    destroy();
}

void Buffer::destroy() noexcept {
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_->nativeHandle(), buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }

    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_->nativeHandle(), memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
}

void Buffer::write(const void* data, VkDeviceSize size) {
    if (size == 0) {
        throw std::invalid_argument{"Vulkan buffer write size must be greater than zero."};
    }

    if (data == nullptr) {
        throw std::invalid_argument{"Vulkan buffer write data pointer must not be null."};
    }

    if (size > size_) {
        throw std::invalid_argument{"Vulkan buffer write size exceeds allocated buffer size."};
    }

    if ((requiredMemoryProperties_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
        throw std::logic_error{"Vulkan buffer memory is not host-visible; cannot write to it."};
    }

    if ((requiredMemoryProperties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        throw std::logic_error{"Buffer::write currently requires host-coherent memory."};
    }

    void* mappedMemory = nullptr;
    const VkResult result = vkMapMemory(device_->nativeHandle(), memory_, 0, size, 0, &mappedMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to map Vulkan buffer memory: VkResult "} + std::to_string(result)};
    }

    std::memcpy(mappedMemory, data, static_cast<std::size_t>(size));
    vkUnmapMemory(device_->nativeHandle(), memory_);
}

Buffer::Buffer(Buffer&& other) noexcept
    : device_{other.device_},
      buffer_{other.buffer_},
      memory_{other.memory_},
      size_{other.size_},
      requiredMemoryProperties_{other.requiredMemoryProperties_} {
    other.device_ = nullptr;
    other.buffer_ = VK_NULL_HANDLE;
    other.memory_ = VK_NULL_HANDLE;
    other.size_ = 0;
    other.requiredMemoryProperties_ = 0;
}


VkBuffer Buffer::nativeHandle() const noexcept {
    return buffer_;
}

}  // namespace ps::renderer::vulkan