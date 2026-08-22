#pragma once

#include <vulkan/vulkan.h>

namespace ps::vulkan {
class Device;
class PhysicalDevice;

/// @brief Owns a 2D Vulkan image and the device-memory allocation bound to it.
///
/// Image deliberately owns only storage. Image views, samplers, layouts and barriers
/// remain responsibilities of the rendering feature using the image.
class Image final {
   public:
    Image(
        const PhysicalDevice& physicalDevice,
        const Device& device,
        VkExtent2D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags requiredMemoryProperties
    );

    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) = delete;

    [[nodiscard]]
    VkImage nativeHandle() const noexcept;

    [[nodiscard]]
    VkExtent2D extent() const noexcept;

    [[nodiscard]]
    VkFormat format() const noexcept;

   private:
    void destroy() noexcept;

    VkDevice device_{VK_NULL_HANDLE};
    VkImage handle_{VK_NULL_HANDLE};
    VkDeviceMemory memory_{VK_NULL_HANDLE};
    VkExtent2D extent_{};
    VkFormat format_{VK_FORMAT_UNDEFINED};
};

}  // namespace ps::vulkan
