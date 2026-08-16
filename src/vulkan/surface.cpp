#include "platform/window.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/surface.hpp"

#include <SDL3/SDL_vulkan.h>
#include <stdexcept>
#include <string>

namespace ps::vulkan {

Surface::Surface(const Instance& instance, const ps::platform::Window& window) : instance_{instance.nativeHandle()} {
    if (!SDL_Vulkan_CreateSurface(window.nativeHandle(), instance_, nullptr, &handle_)) {
        throw std::runtime_error{std::string{"Failed to create Vulkan surface: "} + SDL_GetError()};
    }
}

Surface::~Surface() {
    if (handle_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, handle_, nullptr);
    }
}

VkSurfaceKHR Surface::nativeHandle() const {
    return handle_;
}

}  // namespace ps::vulkan
