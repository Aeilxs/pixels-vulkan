#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace ps::platform {
class Window;
}

namespace ps::vulkan {

class Device;
class PhysicalDevice;
class Surface;

/// @brief Owns the Vulkan swapchain and its presentation image views.
///
/// The swapchain manages the presentable images associated with a window
/// surface and creates one 2D color image view for each image. The swapchain
/// must be recreated when the surface becomes incompatible with its current
/// configuration, such as after a window resize.
///
/// @note The logical Vulkan device used to create this object must outlive it.
class Swapchain final {
   public:
    Swapchain(
        const PhysicalDevice& physicalDevice,
        const Device& device,
        const Surface& surface,
        const platform::Window& window,
        bool uncapped
    );

    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(Swapchain&&) = delete;
    Swapchain& operator=(Swapchain&&) = delete;

    [[nodiscard("The Vulkan swapchain handle must be used")]]
    VkSwapchainKHR nativeHandle() const noexcept;

    [[nodiscard("The Vulkan swapchain images must be used")]]
    const std::vector<VkImage>& images() const noexcept;

    [[nodiscard("The Vulkan swapchain image views must be used")]]
    const std::vector<VkImageView>& imageViews() const noexcept;

    [[nodiscard("The Vulkan swapchain image format must be used")]]
    VkFormat imageFormat() const noexcept;

    [[nodiscard("The Vulkan swapchain extent must be used")]]
    VkExtent2D extent() const noexcept;

    [[nodiscard("The Vulkan swapchain present mode must be used")]]
    VkPresentModeKHR presentMode() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkSwapchainKHR handle_{VK_NULL_HANDLE};

    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;

    VkFormat imageFormat_{VK_FORMAT_UNDEFINED};
    VkExtent2D extent_{};
    VkPresentModeKHR presentMode_{VK_PRESENT_MODE_FIFO_KHR};
};

}  // namespace ps::vulkan
