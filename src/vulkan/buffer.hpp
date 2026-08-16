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
    /// @brief Creates a Vulkan buffer, allocates compatible memory and binds it.
    /// @param physicalDevice Physical device used to select a compatible memory type.
    /// @param device Logical device used to create the buffer and allocate memory.
    /// @param size Requested buffer size in bytes. Must be greater than zero.
    /// @param usage Vulkan usage flags describing how the buffer will be used.
    /// @param requiredMemoryProperties Memory properties required from the selected memory type.
    /// @throws std::invalid_argument If @p size is zero.
    /// @throws std::runtime_error If Vulkan buffer creation, allocation or binding fails.
    Buffer(
        const PhysicalDevice& physicalDevice,
        const Device& device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags requiredMemoryProperties
    );

    /// @brief Releases the owned Vulkan buffer and device memory.
    ~Buffer();
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    /// @brief Transfers ownership of another buffer's Vulkan resources.
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) = delete;

    /// @brief Copies bytes into host-visible, host-coherent buffer memory.
    /// @param data Source bytes to copy. Must not be null.
    /// @param size Number of bytes to write. Must fit in the allocation.
    /// @throws std::invalid_argument If the source pointer or size is invalid.
    /// @throws std::logic_error If the buffer was not created with host-visible
    ///         and host-coherent memory requirements.
    /// @throws std::runtime_error If Vulkan fails to map the memory.
    void write(const void* data, VkDeviceSize size);

    /// @brief Returns the owned Vulkan buffer handle.
    [[nodiscard]]
    VkBuffer nativeHandle() const noexcept;

   private:
    const Device* device_{};

    VkBuffer buffer_{VK_NULL_HANDLE};
    VkDeviceMemory memory_{VK_NULL_HANDLE};
    VkDeviceSize size_{};
    VkMemoryPropertyFlags requiredMemoryProperties_{};

    void destroy() noexcept;
};

}  // namespace ps::vulkan