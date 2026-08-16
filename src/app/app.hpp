#pragma once

#include "app/cli.hpp"
#include "gfx/camera/camera_2d.hpp"
#include "gfx/particles/system.hpp"
#include "platform/sdl_context.hpp"
#include "platform/window.hpp"
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/physical_device.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/surface.hpp"
#include "vulkan/swapchain.hpp"

#include <SDL3/SDL.h>

namespace ps::app {

/// @brief Owns the application lifetime and coordinates input, simulation state and rendering.
///
/// App keeps the long-lived runtime objects in dependency order: particle state, SDL/window
/// resources, the 2D camera, then the Vulkan bootstrap and renderer objects.
class App {
   public:
    App(const cli::AppOptions& options);

    void run();

   private:
    ps::gfx::particles::ParticleSystem particleSystem_;

    ps::platform::SdlContext sdlContext_;
    ps::platform::Window window_;

    ps::gfx::Camera2D camera_;

    ps::vulkan::Instance vulkanInstance_;
    ps::vulkan::Surface vulkanSurface_;
    ps::vulkan::PhysicalDevice physicalDevice_;
    ps::vulkan::Device device_;
    ps::vulkan::Swapchain swapchain_;
    ps::vulkan::Renderer renderer_;

    glm::vec2 mousePosition_{0.0F, 0.0F};

    bool running_{true};

    void pollEvents();
    void handleKeyDown(SDL_Keycode keycode);
    void handleMouseMotion(const SDL_MouseMotionEvent& motion);
};
}  // namespace ps::app
