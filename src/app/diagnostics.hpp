#pragma once

#include "app/cli.hpp"

namespace ps::vulkan {
class PhysicalDevice;
class Swapchain;
}

namespace ps::app::diagnostics {

void logStartup(const cli::AppOptions& options);
void logVulkanRuntime(const ps::vulkan::PhysicalDevice& physicalDevice, const ps::vulkan::Swapchain& swapchain);

}  // namespace ps::app::diagnostics
