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
    /// @brief Creates synchronization primitives for one frame in flight.
    /// @param device Logical device that owns the synchronization objects.
    /// @param swapchainImageCount Number of images owned by the current swapchain.
    /// @throws std::runtime_error If Vulkan cannot create a semaphore or fence.
    FrameSynchronization(const Device& device, std::size_t swapchainImageCount);

    /// @brief Destroys the owned fence and semaphores.
    ~FrameSynchronization();

    FrameSynchronization(const FrameSynchronization&) = delete;
    FrameSynchronization& operator=(const FrameSynchronization&) = delete;

    FrameSynchronization(FrameSynchronization&&) = delete;
    FrameSynchronization& operator=(FrameSynchronization&&) = delete;

    /// @brief Returns the semaphore signaled when a swapchain image becomes available.
    [[nodiscard("The Vulkan image-available semaphore must be used")]]
    VkSemaphore imageAvailable() const noexcept;

    /// @brief Returns the presentation wait semaphore associated with a swapchain image.
    /// @param imageIndex Index returned by vkAcquireNextImageKHR.
    /// @return Render-finished semaphore dedicated to that swapchain image.
    /// @throws std::out_of_range If imageIndex is outside the swapchain image range.
    [[nodiscard("The Vulkan render-finished semaphore must be used")]]
    VkSemaphore renderFinished(std::size_t imageIndex) const;

    /// @brief Returns the fence signaled when the submitted frame finishes on the GPU.
    [[nodiscard("The Vulkan in-flight fence must be used")]]
    VkFence inFlightFence() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkSemaphore imageAvailable_{VK_NULL_HANDLE};
    std::vector<VkSemaphore> renderFinished_;
    VkFence inFlightFence_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
