#pragma once

#include <vulkan/vulkan.h>

namespace ps::vulkan {
class PhysicalDevice;
class Device;

/// @brief Owns a Vulkan buffer and its bound device-memory allocation.
///
/// Buffer centralizes Vulkan resource lifetime and host-visible writes for GPU
/// buffers. It does not know what the bytes represent; usage and
/// memory-property flags are supplied by the caller.
///
/// @note The logical device supplied at construction must outlive this object.
class Buffer {
   public:
    Buffer(
        const PhysicalDevice& physicalDevice,
        const Device& device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags requiredMemoryProperties
    );

    ~Buffer();
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) = delete;

    // Repeated map() calls reuse the existing mapping. Destruction unmaps it.
    void* map();
    void unmap() noexcept;

    // Reuses a persistent mapping when present; otherwise maps temporarily.
    // Non-coherent host memory is intentionally unsupported for now.
    void write(const void* data, VkDeviceSize size);

    [[nodiscard]]
    VkBuffer nativeHandle() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};

    VkBuffer buffer_{VK_NULL_HANDLE};
    VkDeviceMemory memory_{VK_NULL_HANDLE};
    VkDeviceSize size_{};
    VkMemoryPropertyFlags memoryProperties_{};
    void* mappedMemory_{nullptr};

    void destroy() noexcept;
};

}  // namespace ps::vulkan
