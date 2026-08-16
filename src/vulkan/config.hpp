#pragma once

#include <array>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace ps::vulkan::config {
inline constexpr char applicationName[] = "Vulkan - PIXEL STORM";

inline constexpr std::uint32_t applicationVersion = VK_MAKE_VERSION(0, 1, 0);

inline constexpr char engineName[] = "PIXEL STORM Engine";

inline constexpr std::uint32_t engineVersion = VK_MAKE_VERSION(0, 1, 0);

inline constexpr std::uint32_t requiredVulkanApiVersion = VK_API_VERSION_1_3;

inline constexpr std::array requiredVulkanDeviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
}  // namespace ps::vulkan::config
