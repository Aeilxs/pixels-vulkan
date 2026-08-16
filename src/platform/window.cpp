#include "platform/window.hpp"

#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace ps::platform {

Window::Window(const char* title, int width, int height) {
    constexpr SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN;

    handle_ = SDL_CreateWindow(title, width, height, flags);
    if (handle_ == nullptr) {
        throw std::runtime_error("Failed to create SDL window: " + std::string(SDL_GetError()));
    }
}

Window::~Window() {
    if (handle_ != nullptr) {
        SDL_DestroyWindow(handle_);
        handle_ = nullptr;
    }
}

Window::DrawableSize Window::drawableSize() const {
    DrawableSize size{};
    if (!SDL_GetWindowSizeInPixels(handle_, &size.width, &size.height)) {
        throw std::runtime_error("Failed to get SDL window size: " + std::string(SDL_GetError()));
    }

    return size;
}

Window::LogicalSize Window::logicalSize() const {
    LogicalSize size{};
    if (!SDL_GetWindowSize(handle_, &size.width, &size.height)) {
        throw std::runtime_error("Failed to get SDL window size: " + std::string(SDL_GetError()));
    }

    return size;
}

SDL_Window* Window::nativeHandle() const noexcept {
    return handle_;
}

}  // namespace ps::platform
