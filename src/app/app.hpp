#pragma once

#include "app/cli.hpp"
#include "gfx/particles/system.hpp"
#include "platform/sdl_context.hpp"
#include "platform/window.hpp"
#include "renderer/vulkan/device.hpp"
#include "renderer/vulkan/instance.hpp"
#include "renderer/vulkan/physical_device.hpp"
#include "renderer/vulkan/renderer.hpp"
#include "renderer/vulkan/surface.hpp"
#include "renderer/vulkan/swapchain.hpp"

namespace ps::app {
class App {
   public:
    App(const cli::AppOptions& options);

    void run();

   private:
    ps::gfx::particles::ParticleSystem particleSystem_;

    ps::platform::SdlContext sdlContext_;
    ps::platform::Window window_;

    ps::renderer::vulkan::Instance vulkanInstance_;
    ps::renderer::vulkan::Surface vulkanSurface_;
    ps::renderer::vulkan::PhysicalDevice physicalDevice_;
    ps::renderer::vulkan::Device device_;
    ps::renderer::vulkan::Swapchain swapchain_;
    ps::renderer::vulkan::Renderer renderer_;

    bool running_{true};

    void pollEvents();
};
}  // namespace ps::app