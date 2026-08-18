#include "vulkan/config.hpp"
#include "vulkan/debug_messenger.hpp"
#include "vulkan/instance.hpp"

#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ps::vulkan {
namespace {

bool validationLayersAvailable() {
    std::uint32_t layerCount = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{"Failed to enumerate Vulkan instance layers: VkResult " + std::to_string(result)};
    }

    std::vector<VkLayerProperties> availableLayers(layerCount);
    result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{"Failed to enumerate Vulkan instance layers: VkResult " + std::to_string(result)};
    }

    for (const char* requiredLayer : ps::vulkan::config::requiredVulkanValidationLayers) {
        bool found = false;

        for (const VkLayerProperties& availableLayer : availableLayers) {
            if (std::string_view{availableLayer.layerName} == requiredLayer) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

}  // namespace

Instance::Instance() {
    if (config::enableVulkanValidation && !validationLayersAvailable()) {
        throw std::runtime_error{"Required Vulkan validation layers are not available"};
    }

    std::uint32_t extensionCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    if (sdlExtensions == nullptr) {
        throw std::runtime_error{std::string{"Failed to get required Vulkan instance extensions: "} + SDL_GetError()};
    }

    std::vector<const char*> extensions{sdlExtensions, sdlExtensions + extensionCount};
    if (config::enableVulkanValidation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateFlags flags = 0;

#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = config::applicationName;
    applicationInfo.applicationVersion = config::applicationVersion;
    applicationInfo.pEngineName = config::engineName;
    applicationInfo.engineVersion = config::engineVersion;
    applicationInfo.apiVersion = config::requiredVulkanApiVersion;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (config::enableVulkanValidation) {
        createInfo.enabledLayerCount = static_cast<std::uint32_t>(config::requiredVulkanValidationLayers.size());
        createInfo.ppEnabledLayerNames = config::requiredVulkanValidationLayers.data();

        debugCreateInfo = makeDebugMessengerCreateInfo();
        createInfo.pNext = &debugCreateInfo;
    }

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan instance: VkResult "} + std::to_string(result)};
    }
}

Instance::~Instance() {
    if (handle_ != VK_NULL_HANDLE) {
        vkDestroyInstance(handle_, nullptr);
    }
}

VkInstance Instance::nativeHandle() const {
    return handle_;
}

}  // namespace ps::vulkan
