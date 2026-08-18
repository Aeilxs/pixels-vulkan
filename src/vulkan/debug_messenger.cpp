#include "log/log.hpp"
#include "vulkan/config.hpp"
#include "vulkan/debug_messenger.hpp"
#include "vulkan/instance.hpp"

#include <stdexcept>
#include <string>

namespace ps::vulkan {
namespace {

std::string messageTypeName(VkDebugUtilsMessageTypeFlagsEXT messageType) {
    std::string name;

    const auto append = [&name](const char* color, const char* value) {
        if (!name.empty()) {
            name += " | ";
        }

        name += color;
        name += value;
        name += ps::log::color::reset;
    };

    if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0) {
        append(ps::log::color::gray, "General");
    }
    if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0) {
        append(ps::log::color::magenta, "Validation");
    }
    if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0) {
        append(ps::log::color::yellow, "Performance");
    }

    if (name.empty()) {
        append(ps::log::color::gray, "Unknown");
    }

    return name;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*
) {
    const std::string type = messageTypeName(messageType);
    const char* message = callbackData != nullptr && callbackData->pMessage != nullptr ? callbackData->pMessage : "No message provided";

    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            ps::log::trace("\n%s[Vulkan]%s (%s) %s", ps::log::color::cyan, ps::log::color::reset, type.c_str(), message);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            ps::log::debug("\n%s[Vulkan]%s (%s) %s", ps::log::color::cyan, ps::log::color::reset, type.c_str(), message);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            ps::log::warn("\n%s[Vulkan]%s (%s) %s", ps::log::color::cyan, ps::log::color::reset, type.c_str(), message);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            ps::log::error("\n%s[Vulkan]%s (%s) %s", ps::log::color::cyan, ps::log::color::reset, type.c_str(), message);
            break;
        default: ps::log::debug("\n%s[Vulkan]%s (%s) %s", ps::log::color::cyan, ps::log::color::reset, type.c_str(), message); break;
    }

    return VK_FALSE;
}

PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerFunction(VkInstance instance) {
    return reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
}

PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessengerFunction(VkInstance instance) {
    return reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
}

}  // namespace

VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    return createInfo;
}

DebugMessenger::DebugMessenger(const Instance& instance) : instance_{instance.nativeHandle()} {
    if (!config::enableVulkanValidation) {
        return;
    }

    const PFN_vkCreateDebugUtilsMessengerEXT createMessenger = createDebugUtilsMessengerFunction(instance_);
    if (createMessenger == nullptr) {
        throw std::runtime_error{"VK_EXT_debug_utils is enabled but vkCreateDebugUtilsMessengerEXT is unavailable"};
    }

    const VkDebugUtilsMessengerCreateInfoEXT createInfo = makeDebugMessengerCreateInfo();
    const VkResult result = createMessenger(instance_, &createInfo, nullptr, &handle_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{"Failed to create Vulkan debug messenger: VkResult " + std::to_string(result)};
    }
}

DebugMessenger::~DebugMessenger() {
    if (handle_ == VK_NULL_HANDLE) {
        return;
    }

    const PFN_vkDestroyDebugUtilsMessengerEXT destroyMessenger = destroyDebugUtilsMessengerFunction(instance_);
    if (destroyMessenger != nullptr) {
        destroyMessenger(instance_, handle_, nullptr);
    }
}

}  // namespace ps::vulkan
