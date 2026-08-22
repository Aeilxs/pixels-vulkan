#include "vulkan/buffer.hpp"
#include "vulkan/device.hpp"
#include "vulkan/physical_device.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

namespace ps::vulkan {
Buffer::Buffer(
    const PhysicalDevice& physicalDevice,
    const Device& device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags requiredMemoryProperties
)
    : device_{device.nativeHandle()}, size_{size} {
    if (size_ == 0) {
        throw std::invalid_argument{"Vulkan buffer size must be greater than zero."};
    }

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size_;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device_, &bufferCreateInfo, nullptr, &buffer_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan buffer: VkResult "} + std::to_string(result)};
    }

    try {
        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(device_, buffer_, &memoryRequirements);

        const std::uint32_t memoryTypeIndex = physicalDevice.findMemoryType(memoryRequirements.memoryTypeBits, requiredMemoryProperties);
        memoryProperties_ = physicalDevice.memoryTypeProperties(memoryTypeIndex);

        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

        result = vkAllocateMemory(device_, &memoryAllocateInfo, nullptr, &memory_);
        if (result != VK_SUCCESS) {
            throw std::runtime_error{std::string{"Failed to allocate Vulkan memory for buffer: VkResult "} + std::to_string(result)};
        }

        result = vkBindBufferMemory(device_, buffer_, memory_, 0);
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
    unmap();

    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }

    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
}

void* Buffer::map() {
    if ((memoryProperties_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
        throw std::logic_error{"Vulkan buffer memory is not host-visible; cannot map it."};
    }

    if (mappedMemory_ != nullptr) {
        return mappedMemory_;
    }

    const VkResult result = vkMapMemory(device_, memory_, 0, size_, 0, &mappedMemory_);
    if (result != VK_SUCCESS) {
        mappedMemory_ = nullptr;
        throw std::runtime_error{std::string{"Failed to map Vulkan buffer memory: VkResult "} + std::to_string(result)};
    }

    return mappedMemory_;
}

void Buffer::unmap() noexcept {
    if (mappedMemory_ == nullptr) {
        return;
    }

    vkUnmapMemory(device_, memory_);
    mappedMemory_ = nullptr;
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

    if ((memoryProperties_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
        throw std::logic_error{"Vulkan buffer memory is not host-visible; cannot write to it."};
    }

    // Non-coherent host memory requires explicit vkFlushMappedMemoryRanges calls.
    // Keep write() honest until that path is implemented.
    if ((memoryProperties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        throw std::logic_error{"Buffer::write currently requires host-coherent memory."};
    }

    const bool wasMapped = mappedMemory_ != nullptr;
    void* destination = map();
    std::memcpy(destination, data, static_cast<std::size_t>(size));

    if (!wasMapped) {
        unmap();
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : device_{other.device_},
      buffer_{other.buffer_},
      memory_{other.memory_},
      size_{other.size_},
      memoryProperties_{other.memoryProperties_},
      mappedMemory_{other.mappedMemory_} {
    other.device_ = VK_NULL_HANDLE;
    other.buffer_ = VK_NULL_HANDLE;
    other.memory_ = VK_NULL_HANDLE;
    other.size_ = 0;
    other.memoryProperties_ = 0;
    other.mappedMemory_ = nullptr;
}

VkBuffer Buffer::nativeHandle() const noexcept {
    return buffer_;
}

}  // namespace ps::vulkan
