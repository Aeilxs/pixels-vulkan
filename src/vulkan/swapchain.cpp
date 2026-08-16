#include "platform/window.hpp"
#include "vulkan/device.hpp"
#include "vulkan/physical_device.hpp"
#include "vulkan/surface.hpp"
#include "vulkan/swapchain.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace platform = ps::platform;

namespace {

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

[[nodiscard("The Vulkan swapchain support details must be used")]]
SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    SwapchainSupportDetails details{};

    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to query Vulkan surface capabilities: VkResult "} + std::to_string(result)};
    }

    // We need to query the number of formats and present modes first, then allocate space for them, and finally query
    // the actual data. It's done this way because the vulkan API doesn't like to allocate memory for you, so you have
    // to do it yourself.
    std::uint32_t formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to count Vulkan surface formats: VkResult "} + std::to_string(result)};
    }

    details.formats.resize(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to enumerate Vulkan surface formats: VkResult "} + std::to_string(result)};
    }

    // We also need to query the number of present modes, then allocate space for them, and finally query the actual
    // data. For the same reason as above, the vulkan API doesn't like to allocate memory for you, so you have to do it
    // yourself.
    std::uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to count Vulkan surface present modes: VkResult "} + std::to_string(result)};
    }

    details.presentModes.resize(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to enumerate Vulkan surface present modes: VkResult "} + std::to_string(result)};
    }

    return details;
}

[[nodiscard("The selected Vulkan surface format must be used")]]
VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    constexpr VkFormat preferredFormats[] = {
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_SRGB,
    };

    for (const VkFormat preferredFormat : preferredFormats) {
        for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
            if (availableFormat.format == preferredFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
    }

    throw std::runtime_error{"No supported sRGB swapchain surface format"};
}

[[nodiscard("The selected Vulkan present mode must be used")]]
VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Return VK_PRESENT_MODE_MAILBOX_KHR if available, otherwise return VK_PRESENT_MODE_FIFO_KHR.
    for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available, so we can return it if MAILBOX unavailable.
    return VK_PRESENT_MODE_FIFO_KHR;
}

[[nodiscard("The selected Vulkan swapchain extent must be used")]]
VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, const platform::Window& window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    const platform::Window::DrawableSize size = window.drawableSize();
    const std::uint32_t w = static_cast<std::uint32_t>(size.width);
    const std::uint32_t h = static_cast<std::uint32_t>(size.height);
    VkExtent2D extent{
        .width = std::clamp(w, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = std::clamp(h, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };

    return extent;
}

[[nodiscard("The selected Vulkan composite alpha mode must be used")]]
VkCompositeAlphaFlagBitsKHR chooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) {
    if ((capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) != 0) {
        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    if ((capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) != 0) {
        return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    }
    if ((capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) != 0) {
        return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }
    if ((capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) != 0) {
        return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }

    throw std::runtime_error{"Failed to find a supported Vulkan composite alpha mode"};
}

[[nodiscard("The created Vulkan image view must be used")]]
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.image = image;
    createInfo.format = format;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;

    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = VK_NULL_HANDLE;

    const VkResult result = vkCreateImageView(device, &createInfo, nullptr, &imageView);

    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan image view: VkResult "} + std::to_string(result)};
    }

    return imageView;
}

}  // namespace

namespace ps::vulkan {

Swapchain::Swapchain(const PhysicalDevice& physicalDevice, const Device& device, const Surface& surface, const platform::Window& window)
    : device_(device.nativeHandle()) {
    const SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice.nativeHandle(), surface.nativeHandle());
    const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapchainSupport.formats);
    const VkPresentModeKHR presentMode = choosePresentMode(swapchainSupport.presentModes);
    const VkExtent2D extent = chooseExtent(swapchainSupport.capabilities, window);

    std::uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface.nativeHandle();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if ((swapchainSupport.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0) {
        throw std::runtime_error{"Vulkan surface does not support swapchain images as color attachments"};
    }

    const QueueFamilyIndices& indices = physicalDevice.queueFamilies();
    const std::uint32_t graphicsFamily = indices.graphics.value();
    const std::uint32_t presentFamily = indices.present.value();
    const std::uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};

    if (graphicsFamily != presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;

    createInfo.compositeAlpha = chooseCompositeAlpha(swapchainSupport.capabilities);

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device_, &createInfo, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{"Failed to create Vulkan swapchain: VkResult " + std::to_string(result)};
    }

    imageFormat_ = surfaceFormat.format;
    extent_ = extent;

    try {
        std::uint32_t actualImageCount = 0;

        result = vkGetSwapchainImagesKHR(device_, handle_, &actualImageCount, nullptr);

        if (result != VK_SUCCESS) {
            throw std::runtime_error{std::string{"Failed to count Vulkan swapchain images: VkResult "} + std::to_string(result)};
        }

        images_.resize(actualImageCount);

        result = vkGetSwapchainImagesKHR(device_, handle_, &actualImageCount, images_.data());

        if (result != VK_SUCCESS) {
            throw std::runtime_error{std::string{"Failed to retrieve Vulkan swapchain images: VkResult "} + std::to_string(result)};
        }

        imageViews_.reserve(images_.size());
        for (const VkImage image : images_) {
            imageViews_.push_back(createImageView(device_, image, imageFormat_));
        }
    } catch (...) {
        for (const VkImageView imageView : imageViews_) {
            vkDestroyImageView(device_, imageView, nullptr);
        }

        imageViews_.clear();
        vkDestroySwapchainKHR(device_, handle_, nullptr);
        handle_ = VK_NULL_HANDLE;
        throw;
    }
}

Swapchain::~Swapchain() {
    for (const VkImageView imageView : imageViews_) {
        vkDestroyImageView(device_, imageView, nullptr);
    }

    if (handle_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, handle_, nullptr);
    }
}

VkSwapchainKHR Swapchain::nativeHandle() const noexcept {
    return handle_;
}

const std::vector<VkImage>& Swapchain::images() const noexcept {
    return images_;
}

const std::vector<VkImageView>& Swapchain::imageViews() const noexcept {
    return imageViews_;
}

VkFormat Swapchain::imageFormat() const noexcept {
    return imageFormat_;
}

VkExtent2D Swapchain::extent() const noexcept {
    return extent_;
}

}  // namespace ps::vulkan