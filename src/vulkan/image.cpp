#include "vulkan/image.hpp"

#include "vulkan/device.hpp"
#include "vulkan/physical_device.hpp"

#include <stdexcept>
#include <string>

namespace ps::vulkan {

Image::Image(
    const PhysicalDevice& physicalDevice,
    const Device& device,
    VkExtent2D extent,
    VkFormat format,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags requiredMemoryProperties
)
    : device_{device.nativeHandle()}, extent_{extent}, format_{format} {
    if (extent_.width == 0 || extent_.height == 0) {
        throw std::invalid_argument{"Vulkan image extent must be greater than zero."};
    }

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent = {extent_.width, extent_.height, 1};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format_;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(device_, &imageCreateInfo, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan image: VkResult "} + std::to_string(result)};
    }

    try {
        VkMemoryRequirements memoryRequirements{};
        vkGetImageMemoryRequirements(device_, handle_, &memoryRequirements);

        VkMemoryAllocateInfo allocationInfo{};
        allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.allocationSize = memoryRequirements.size;
        allocationInfo.memoryTypeIndex = physicalDevice.findMemoryType(memoryRequirements.memoryTypeBits, requiredMemoryProperties);

        result = vkAllocateMemory(device_, &allocationInfo, nullptr, &memory_);
        if (result != VK_SUCCESS) {
            throw std::runtime_error{std::string{"Failed to allocate Vulkan image memory: VkResult "} + std::to_string(result)};
        }

        result = vkBindImageMemory(device_, handle_, memory_, 0);
        if (result != VK_SUCCESS) {
            throw std::runtime_error{std::string{"Failed to bind Vulkan image memory: VkResult "} + std::to_string(result)};
        }
    } catch (...) {
        destroy();
        throw;
    }
}

Image::~Image() {
    destroy();
}

Image::Image(Image&& other) noexcept
    : device_{other.device_}, handle_{other.handle_}, memory_{other.memory_}, extent_{other.extent_}, format_{other.format_} {
    other.device_ = VK_NULL_HANDLE;
    other.handle_ = VK_NULL_HANDLE;
    other.memory_ = VK_NULL_HANDLE;
    other.extent_ = {};
    other.format_ = VK_FORMAT_UNDEFINED;
}

void Image::destroy() noexcept {
    if (handle_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_, handle_, nullptr);
        handle_ = VK_NULL_HANDLE;
    }

    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
}

VkImage Image::nativeHandle() const noexcept {
    return handle_;
}

VkExtent2D Image::extent() const noexcept {
    return extent_;
}

VkFormat Image::format() const noexcept {
    return format_;
}

}  // namespace ps::vulkan
