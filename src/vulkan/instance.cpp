#include "app/config.hpp"
#include "vulkan/config.hpp"
#include "vulkan/instance.hpp"

#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace ps::vulkan {

Instance::Instance() {
    std::uint32_t extensionCount = 0;

    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    if (sdlExtensions == nullptr) {
        throw std::runtime_error{std::string{"Failed to get required Vulkan instance extensions: "} + SDL_GetError()};
    }

    std::vector<const char*> extensions{sdlExtensions, sdlExtensions + extensionCount};
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
