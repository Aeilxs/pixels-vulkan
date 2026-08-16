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
    /// @brief Creates a swapchain for the selected physical device and surface.
    /// @param physicalDevice Selected physical device used to query surface support.
    /// @param device Logical device that owns the swapchain and image views.
    /// @param surface Presentation surface associated with the window.
    /// @param window Window used to determine the drawable extent when required.
    /// @throws std::runtime_error If the surface configuration is unsupported or Vulkan creation fails.
    Swapchain(const PhysicalDevice& physicalDevice, const Device& device, const Surface& surface, const platform::Window& window);

    /// @brief Destroys the image views and the owned Vulkan swapchain.
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(Swapchain&&) = delete;
    Swapchain& operator=(Swapchain&&) = delete;

    /// @brief Returns the native Vulkan swapchain handle without transferring ownership.
    /// @return The owned swapchain handle, valid for this object's lifetime.
    [[nodiscard("The Vulkan swapchain handle must be used")]]
    VkSwapchainKHR nativeHandle() const noexcept;

    /// @brief Returns the presentable images owned by the Vulkan swapchain.
    /// @return Non-owning image handles indexed in swapchain image order.
    [[nodiscard("The Vulkan swapchain images must be used")]]
    const std::vector<VkImage>& images() const noexcept;

    /// @brief Returns the image views created for the swapchain images.
    /// @return Owned image-view handles indexed in the same order as images().
    [[nodiscard("The Vulkan swapchain image views must be used")]]
    const std::vector<VkImageView>& imageViews() const noexcept;

    /// @brief Returns the format used by the swapchain images.
    /// @return Selected Vulkan image format.
    [[nodiscard("The Vulkan swapchain image format must be used")]]
    VkFormat imageFormat() const noexcept;

    /// @brief Returns the current swapchain image extent in physical pixels.
    /// @return Selected swapchain width and height.
    [[nodiscard("The Vulkan swapchain extent must be used")]]
    VkExtent2D extent() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkSwapchainKHR handle_{VK_NULL_HANDLE};

    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;

    VkFormat imageFormat_{VK_FORMAT_UNDEFINED};
    VkExtent2D extent_{};
};

}  // namespace ps::vulkan
