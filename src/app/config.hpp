#pragma once

#include <filesystem>

#ifndef PIXEL_STORM_ASSET_DIR
#error "PIXEL_STORM_ASSET_DIR must be defined by CMake"
#endif

namespace ps::app::config {

inline constexpr char windowName[] = "Vulkan - PIXEL STORM";
inline constexpr int initialWindowWidth = 1200;
inline constexpr int initialWindowHeight = 800;

inline const std::filesystem::path overlayFontPath{
    PIXEL_STORM_ASSET_DIR "/fonts/jetbrains/JetBrainsMonoNerdFont-Regular.ttf"
};
inline constexpr float overlayFontPixelHeight = 32.0F;

}  // namespace ps::app::config
