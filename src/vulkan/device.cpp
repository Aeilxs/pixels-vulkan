#include "app/config.hpp"
#include "vulkan/config.hpp"
#include "vulkan/device.hpp"
#include "vulkan/physical_device.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace ps::vulkan {

Device::Device(const PhysicalDevice& physicalDevice) {
    const QueueFamilyIndices& queueFamilies = physicalDevice.queueFamilies();
    const std::uint32_t graphicsFamily = queueFamilies.graphics.value();
    const std::uint32_t presentFamily = queueFamilies.present.value();

    std::vector<std::uint32_t> uniqueQueueFamilies{
        graphicsFamily,
    };

    if (presentFamily != graphicsFamily) {
        uniqueQueueFamilies.push_back(presentFamily);
    }

    constexpr float queuePriority = 1.0F;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());

    for (const std::uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.features.largePoints = VK_TRUE;
    features.pNext = &features13;

    std::vector<const char*> enabledExtensions{
        config::requiredVulkanDeviceExtensions.begin(),
        config::requiredVulkanDeviceExtensions.end(),
    };

#ifdef __APPLE__
    enabledExtensions.push_back("VK_KHR_portability_subset");
#endif

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features;
    createInfo.pEnabledFeatures = nullptr;  // Features are specified in the pNext chain
    createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    const VkResult result = vkCreateDevice(physicalDevice.nativeHandle(), &createInfo, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan logical device: VkResult "} + std::to_string(result)};
    }

    vkGetDeviceQueue(handle_, graphicsFamily, 0, &graphicsQueue_);
    vkGetDeviceQueue(handle_, presentFamily, 0, &presentQueue_);
}

VkQueue Device::graphicsQueue() const noexcept {
    return graphicsQueue_;
}

VkQueue Device::presentQueue() const noexcept {
    return presentQueue_;
}

VkDevice Device::nativeHandle() const noexcept {
    return handle_;
}

Device::~Device() {
    if (handle_ != VK_NULL_HANDLE) {
        vkDestroyDevice(handle_, nullptr);
    }
}

}  // namespace ps::vulkan
