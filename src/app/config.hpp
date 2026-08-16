#pragma once

#include <array>
#include <cstdint>
#include <vulkan/vulkan.h>

/// @brief Compile-time configuration shared by the application and renderer.
namespace ps::app::config {

/// @brief Name reported to the window system and Vulkan implementation.
inline constexpr char windowName[] = "Vulkan - PIXEL STORM";

/// @brief Initial width of the application window in logical pixels.
inline constexpr int initialWindowWidth = 1600;

/// @brief Initial height of the application window in logical pixels.
inline constexpr int initialWindowHeight = 1200;

}  // namespace ps::app::config
