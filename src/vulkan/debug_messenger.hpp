#pragma once

#include <vulkan/vulkan.h>

namespace ps::vulkan {

class Instance;

[[nodiscard]] VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo();

/// @brief Owns the Vulkan debug messenger used by validation/debug utilities.
///
/// The messenger is only created when Vulkan validation is enabled. It must be
/// destroyed before the VkInstance it belongs to.
class DebugMessenger final {
   public:
    explicit DebugMessenger(const Instance& instance);
    ~DebugMessenger();

    DebugMessenger(const DebugMessenger&) = delete;
    DebugMessenger& operator=(const DebugMessenger&) = delete;

    DebugMessenger(DebugMessenger&&) = delete;
    DebugMessenger& operator=(DebugMessenger&&) = delete;

   private:
    VkInstance instance_{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
