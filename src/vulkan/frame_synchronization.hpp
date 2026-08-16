#pragma once

#include <cstddef>
#include <vector>
#include <vulkan/vulkan.h>

namespace ps::vulkan {

class Device;

/// @brief Owns the synchronization primitives used by the single-frame-in-flight renderer.
///
/// One semaphore signals swapchain image acquisition, one fence prevents the CPU from
/// re-recording the command buffer while the GPU is still using it, and one render-finished
/// semaphore is created per swapchain image so presentation semaphores are reused safely.
///
/// @note The logical Vulkan device used to create these objects must outlive this object.
class FrameSynchronization final {
   public:
    FrameSynchronization(const Device& device, std::size_t swapchainImageCount);

    ~FrameSynchronization();

    FrameSynchronization(const FrameSynchronization&) = delete;
    FrameSynchronization& operator=(const FrameSynchronization&) = delete;

    FrameSynchronization(FrameSynchronization&&) = delete;
    FrameSynchronization& operator=(FrameSynchronization&&) = delete;

    [[nodiscard("The Vulkan image-available semaphore must be used")]]
    VkSemaphore imageAvailable() const noexcept;

    [[nodiscard("The Vulkan render-finished semaphore must be used")]]
    VkSemaphore renderFinished(std::size_t imageIndex) const;

    [[nodiscard("The Vulkan in-flight fence must be used")]]
    VkFence inFlightFence() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkSemaphore imageAvailable_{VK_NULL_HANDLE};
    std::vector<VkSemaphore> renderFinished_;
    VkFence inFlightFence_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
