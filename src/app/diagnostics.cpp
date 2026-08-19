#include "app/config.hpp"
#include "app/diagnostics.hpp"
#include "build/build_info.hpp"
#include "log/log.hpp"
#include "vulkan/config.hpp"
#include "vulkan/physical_device.hpp"
#include "vulkan/swapchain.hpp"

#include <ostream>
#include <sstream>

namespace ps::app::diagnostics {
namespace {

void appendBuildInfo(std::ostream& output) {
    output << "Build information\n"
           << "-----------------\n"
           << "Application: " << ps::build::name << '\n'
           << "Version: " << ps::build::version << '\n'
           << "Configuration: " << ps::build::configuration << '\n'
           << "Branch: " << ps::build::gitBranch << '\n'
           << "Commit: @" << ps::build::gitHash << (ps::build::gitDirty ? " (dirty)" : "") << '\n'
           << "Compiler: " << ps::build::compiler << '\n';
}

void appendApplicationConfiguration(std::ostream& output, const cli::AppOptions& options) {
    output << "Application configuration\n"
           << "-------------------------\n"
           << "Image: " << options.image_path.string() << '\n'
           << "Gap: " << options.gap << '\n'
           << "Window title: " << config::windowName << '\n'
           << "Initial window size: " << config::initialWindowWidth << 'x' << config::initialWindowHeight << '\n'
           << "Log level: " << ps::log::configuredLevelName() << '\n';
}

const char* presentModeName(VkPresentModeKHR presentMode) noexcept {
    switch (presentMode) {
        case VK_PRESENT_MODE_MAILBOX_KHR: return "VK_PRESENT_MODE_MAILBOX_KHR";
        case VK_PRESENT_MODE_FIFO_KHR: return "VK_PRESENT_MODE_FIFO_KHR";
        default: return "unknown";
    }
}

void appendVulkanConfiguration(std::ostream& output) {
    output << "Vulkan configuration\n"
           << "--------------------\n"
           << "Application: " << ps::vulkan::config::applicationName << '\n'
           << "Application version: " << VK_VERSION_MAJOR(ps::vulkan::config::applicationVersion) << '.'
           << VK_VERSION_MINOR(ps::vulkan::config::applicationVersion) << '.' << VK_VERSION_PATCH(ps::vulkan::config::applicationVersion) << '\n'
           << "Engine: " << ps::vulkan::config::engineName << '\n'
           << "Engine version: " << VK_VERSION_MAJOR(ps::vulkan::config::engineVersion) << '.' << VK_VERSION_MINOR(ps::vulkan::config::engineVersion)
           << '.' << VK_VERSION_PATCH(ps::vulkan::config::engineVersion) << '\n'
           << "Required Vulkan API: " << VK_API_VERSION_MAJOR(ps::vulkan::config::requiredVulkanApiVersion) << '.'
           << VK_API_VERSION_MINOR(ps::vulkan::config::requiredVulkanApiVersion) << '.'
           << VK_API_VERSION_PATCH(ps::vulkan::config::requiredVulkanApiVersion) << '\n'
           << "Validation layers: " << (ps::vulkan::config::enableVulkanValidation ? "enabled" : "disabled") << '\n';

    output << "Required device extensions:";
    for (const char* extension : ps::vulkan::config::requiredVulkanDeviceExtensions) {
        output << "\n  - " << extension;
    }
    output << '\n';

    if (ps::vulkan::config::enableVulkanValidation) {
        output << "Required validation layers:";
        for (const char* layer : ps::vulkan::config::requiredVulkanValidationLayers) {
            output << "\n  - " << layer;
        }
        output << '\n';
    }
}

}  // namespace

void logStartup(const cli::AppOptions& options) {
    std::ostringstream output;

    output << "\n\n";

    appendBuildInfo(output);
    output << '\n';

    appendApplicationConfiguration(output, options);
    output << '\n';

    appendVulkanConfiguration(output);

    output << '\n';

    ps::log::info("%s", output.str().c_str());
}

void logVulkanRuntime(const ps::vulkan::PhysicalDevice& physicalDevice, const ps::vulkan::Swapchain& swapchain) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice.nativeHandle(), &properties);

    const VkExtent2D extent = swapchain.extent();

    std::ostringstream output;
    output << "\n\nRuntime Vulkan\n"
           << "--------------\n"
           << "Physical device: " << properties.deviceName << '\n'
           << "Swapchain extent: " << extent.width << 'x' << extent.height << '\n'
           << "Swapchain images: " << swapchain.images().size() << '\n'
           << "Present mode: " << presentModeName(swapchain.presentMode()) << '\n';

    ps::log::info("%s", output.str().c_str());
}

}  // namespace ps::app::diagnostics