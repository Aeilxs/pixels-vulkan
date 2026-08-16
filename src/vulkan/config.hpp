#include <array>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace ps::vulkan::config {
/// @brief Name reported to the window system and Vulkan implementation.
inline constexpr char applicationName[] = "Vulkan - PIXEL STORM";

/// @brief Application version encoded with Vulkan's version format.
inline constexpr std::uint32_t applicationVersion = VK_MAKE_VERSION(0, 1, 0);

/// @brief Name of the rendering engine reported to Vulkan.
inline constexpr char engineName[] = "PIXEL STORM Engine";

/// @brief Engine version encoded with Vulkan's version format.
inline constexpr std::uint32_t engineVersion = VK_MAKE_VERSION(0, 1, 0);

/// @brief Minimum Vulkan API version required by the renderer.
inline constexpr std::uint32_t requiredVulkanApiVersion = VK_API_VERSION_1_3;

/// @brief Vulkan device extensions required by the renderer.
inline constexpr std::array requiredVulkanDeviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
}  // namespace ps::vulkan::config