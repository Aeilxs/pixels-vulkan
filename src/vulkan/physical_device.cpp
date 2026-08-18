#include "vulkan/config.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/physical_device.hpp"
#include "vulkan/queue_family_indices.hpp"
#include "vulkan/surface.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using ps::vulkan::QueueFamilyIndices;

[[nodiscard]]
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::uint32_t familyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());
    QueueFamilyIndices indices;

    for (std::uint32_t index = 0; index < familyCount; ++index) {
        const VkQueueFamilyProperties& family = families[index];

        const bool supportsGraphics = family.queueCount > 0 && (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;

        if (!indices.graphics.has_value() && supportsGraphics) {
            indices.graphics = index;
        }

        VkBool32 supportsPresentation = VK_FALSE;

        const VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &supportsPresentation);

        if (result != VK_SUCCESS) {
            throw std::runtime_error{std::string{"Failed to query Vulkan presentation support: VkResult "} + std::to_string(result)};
        }

        const bool hasPresentationQueue = family.queueCount > 0 && supportsPresentation == VK_TRUE;

        if (!indices.present.has_value() && hasPresentationQueue) {
            indices.present = index;
        }

        if (indices.complete()) {
            break;
        }
    }

    return indices;
}

[[nodiscard]]
bool supportsRequiredExtensions(VkPhysicalDevice device) {
    std::uint32_t extensionCount = 0;

    VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to count Vulkan device extensions: VkResult "} + std::to_string(result)};
    }

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);

    result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to enumerate Vulkan device extensions: VkResult "} + std::to_string(result)};
    }

    for (const char* requiredExtension : ps::vulkan::config::requiredVulkanDeviceExtensions) {
        bool extensionFound = false;

        for (const VkExtensionProperties& availableExtension : availableExtensions) {
            const std::string_view availableName{availableExtension.extensionName};

            if (availableName == requiredExtension) {
                extensionFound = true;
                break;
            }
        }

        if (!extensionFound) {
            return false;
        }
    }

    return true;
}

[[nodiscard]]
bool hasAdequateSwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::uint32_t formatCount = 0;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to count Vulkan surface formats: VkResult "} + std::to_string(result)};
    }

    std::uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to count Vulkan present modes: VkResult "} + std::to_string(result)};
    }

    return formatCount > 0 && presentModeCount > 0;
}

struct RequiredFeatures {
    bool dynamicRendering = false;
    bool synchronization2 = false;
    bool largePoints = false;
    bool shaderDemoteToHelperInvocation = false;
};

[[nodiscard]]
RequiredFeatures queryRequiredFeatures(VkPhysicalDevice device) {
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &features13;

    vkGetPhysicalDeviceFeatures2(device, &features);

    return RequiredFeatures{
        .dynamicRendering = features13.dynamicRendering == VK_TRUE,
        .synchronization2 = features13.synchronization2 == VK_TRUE,
        .largePoints = features.features.largePoints == VK_TRUE,
        .shaderDemoteToHelperInvocation = features13.shaderDemoteToHelperInvocation == VK_TRUE,
    };
}

[[nodiscard]]
bool supportsRequiredApiVersion(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device, &properties);

    return properties.apiVersion >= ps::vulkan::config::requiredVulkanApiVersion;
}

struct DeviceEvaluation {
    bool supportsApiVersion = false;
    QueueFamilyIndices queueFamilies;
    bool supportsExtensions = false;
    bool hasAdequateSwapchain = false;
    RequiredFeatures requiredFeatures;

    [[nodiscard]]
    // clang-format off
    bool suitable() const noexcept {
        return supportsApiVersion                            && 
            queueFamilies.complete()                         && 
            supportsExtensions                               &&
            hasAdequateSwapchain                             &&
            requiredFeatures.dynamicRendering                &&
            requiredFeatures.synchronization2                &&
            requiredFeatures.largePoints                     &&
            requiredFeatures.shaderDemoteToHelperInvocation;
    }
    // clang-format on
};

[[nodiscard]]
DeviceEvaluation evaluateDevice(VkPhysicalDevice device, VkSurfaceKHR surface) {
    DeviceEvaluation evaluation;

    evaluation.supportsApiVersion = supportsRequiredApiVersion(device);

    if (!evaluation.supportsApiVersion) {
        return evaluation;
    }

    evaluation.queueFamilies = findQueueFamilies(device, surface);
    if (!evaluation.queueFamilies.complete()) {
        return evaluation;
    }

    evaluation.supportsExtensions = supportsRequiredExtensions(device);
    if (!evaluation.supportsExtensions) {
        return evaluation;
    }

    evaluation.hasAdequateSwapchain = hasAdequateSwapchainSupport(device, surface);
    if (!evaluation.hasAdequateSwapchain) {
        return evaluation;
    }

    evaluation.requiredFeatures = queryRequiredFeatures(device);
    return evaluation;
}

}  // namespace

namespace ps::vulkan {

PhysicalDevice::PhysicalDevice(const Instance& instance, const Surface& surface) {
    std::uint32_t deviceCount = 0;
    const VkResult countResult = vkEnumeratePhysicalDevices(instance.nativeHandle(), &deviceCount, nullptr);
    if (countResult != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to count Vulkan physical devices: VkResult "} + std::to_string(countResult)};
    }
    if (deviceCount == 0) {
        throw std::runtime_error{"Failed to find any Vulkan physical device"};
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);

    const VkResult enumerationResult = vkEnumeratePhysicalDevices(instance.nativeHandle(), &deviceCount, devices.data());
    if (enumerationResult != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to enumerate Vulkan physical devices: VkResult "} + std::to_string(enumerationResult)};
    }

    for (VkPhysicalDevice device : devices) {
        const DeviceEvaluation evaluation = evaluateDevice(device, surface.nativeHandle());
        if (!evaluation.suitable()) {
            continue;
        }

        handle_ = device;
        queueFamilies_ = evaluation.queueFamilies;
        break;
    }

    if (handle_ == VK_NULL_HANDLE) {
        throw std::runtime_error{"Failed to find a suitable Vulkan physical device"};
    }

    vkGetPhysicalDeviceMemoryProperties(handle_, &memoryProperties_);
}

std::uint32_t PhysicalDevice::findMemoryType(std::uint32_t filter, VkMemoryPropertyFlags properties) const {
    for (std::uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
        if ((filter & (1U << i)) && (memoryProperties_.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error{"Failed to find suitable Vulkan memory type"};
}

const QueueFamilyIndices& PhysicalDevice::queueFamilies() const {
    return queueFamilies_;
}

VkPhysicalDevice PhysicalDevice::nativeHandle() const {
    return handle_;
}

}  // namespace ps::vulkan
