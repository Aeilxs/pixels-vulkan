#pragma once

#include "app/cli.hpp"
#include "gfx/camera/camera_2d.hpp"
#include "gfx/particles/system.hpp"
#include "platform/sdl_context.hpp"
#include "platform/window.hpp"
#include "renderer/vulkan/device.hpp"
#include "renderer/vulkan/instance.hpp"
#include "renderer/vulkan/physical_device.hpp"
#include "renderer/vulkan/renderer.hpp"
#include "renderer/vulkan/surface.hpp"
#include "renderer/vulkan/swapchain.hpp"

#include <SDL3/SDL.h>

namespace ps::app {

/// @brief Owns the application lifetime and coordinates input, simulation state and rendering.
///
/// App keeps the long-lived runtime objects in dependency order: particle state, SDL/window
/// resources, the 2D camera, then the Vulkan bootstrap and renderer objects.
class App {
   public:
    /// @brief Builds the runtime state from the validated command-line options.
    /// @param options Startup options used to load the initial particle image.
    App(const cli::AppOptions& options);

    /// @brief Runs the event/render loop until the application is asked to stop.
    void run();

   private:
    ps::gfx::particles::ParticleSystem particleSystem_;

    ps::platform::SdlContext sdlContext_;
    ps::platform::Window window_;

    ps::gfx::Camera2D camera_;

    ps::renderer::vulkan::Instance vulkanInstance_;
    ps::renderer::vulkan::Surface vulkanSurface_;
    ps::renderer::vulkan::PhysicalDevice physicalDevice_;
    ps::renderer::vulkan::Device device_;
    ps::renderer::vulkan::Swapchain swapchain_;
    ps::renderer::vulkan::Renderer renderer_;

    glm::vec2 mousePosition_{0.0F, 0.0F};

    bool running_{true};

    void pollEvents();
    void handleKeyDown(SDL_Keycode keycode);
    glm::vec2 mousePosition(const SDL_MouseMotionEvent& motion);
};
}  // namespace ps::app
