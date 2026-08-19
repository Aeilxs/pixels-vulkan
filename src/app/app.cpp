#include "app/app.hpp"
#include "app/config.hpp"
#include "app/diagnostics.hpp"
#include "log/log.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>

namespace {
// Use the actual logical window size; the window manager may adjust the size requested at creation.
glm::vec2 windowLogicalSize(const ps::platform::Window& window) {
    const ps::platform::Window::LogicalSize size = window.logicalSize();

    return {
        static_cast<float>(size.width),
        static_cast<float>(size.height),
    };
}
}  // namespace

namespace ps::app {
App::App(const cli::AppOptions& options)
    : particleSystem_{gfx::particles::ParticleSystem::fromImage(ps::image::load(options.image_path), options.gap)},
      sdlContext_{},
      window_{config::windowName, config::initialWindowWidth, config::initialWindowHeight},
      camera_{windowLogicalSize(window_), 1.0F},
      vulkanInstance_{},
      vulkanDebugMessenger_{vulkanInstance_},
      vulkanSurface_{vulkanInstance_, window_},
      physicalDevice_{vulkanInstance_, vulkanSurface_},
      device_{physicalDevice_},
      swapchain_{physicalDevice_, device_, vulkanSurface_, window_},
      renderer_{physicalDevice_, device_, swapchain_, particleSystem_.particles()} {
    const glm::vec2 contentSize{
        static_cast<float>(particleSystem_.imageDimensions().width),
        static_cast<float>(particleSystem_.imageDimensions().height),
    };

    diagnostics::logVulkanRuntime(physicalDevice_, swapchain_);

    ps::log::trace("Run with image: %s, gap: %d", options.image_path.string().c_str(), options.gap);
    ps::log::trace("Particle system content size: %.1f x %.1f", contentSize.x, contentSize.y);
    ps::log::trace("Particle system particle count: %zu", particleSystem_.particles().size());

    camera_.fit(contentSize * 0.5F, contentSize, 0.95F);
}

void App::run() {
    auto previousTime = std::chrono::steady_clock::now();

    while (running_) {
        const auto frameWallStartTime = std::chrono::steady_clock::now();

        const float dt = std::min(std::chrono::duration<float>(frameWallStartTime - previousTime).count(), 0.05F);
        previousTime = frameWallStartTime;

        pollEvents();
        if (!running_) {
            break;
        }

        const auto simulationStartTime = std::chrono::steady_clock::now();
        particleSystem_.update(dt, camera_.screenToWorld(mousePosition_));
        const auto simulationTime = std::chrono::steady_clock::now() - simulationStartTime;

        const ps::vulkan::FrameTimings rendererTimings = renderer_.drawFrame(camera_.viewProjection(), particleSystem_.particles());
        const FrameSample sample{
            std::chrono::steady_clock::now() - frameWallStartTime,
            simulationTime,
            rendererTimings.particleUploadTime,
            rendererTimings.fenceWaitTime,
        };
        if (frameStats_.push(sample)) {
            const auto& metrics = frameStats_.metrics();

            ps::log::trace(
                "FPS %.1f | Frame wall %.2f ms | Simulation %.2f ms | Upload %.2f ms | Fence %.2f ms",
                metrics.fps,
                metrics.frameWallTimeMs,
                metrics.simulationTimeMs,
                metrics.particleUploadTimeMs,
                metrics.fenceWaitTimeMs
            );
        }
    }
}

void App::pollEvents() {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: running_ = false; break;
            case SDL_EVENT_MOUSE_MOTION: handleMouseMotion(event.motion); break;
            case SDL_EVENT_KEY_DOWN: handleKeyDown(event.key.key); break;
            default: break;
        }
    }
}

void App::handleKeyDown(SDL_Keycode keycode) {
    switch (keycode) {
        case SDLK_Q:
        case SDLK_ESCAPE: running_ = false; break;

        case SDLK_UP: camera_.setCenter(camera_.center() + glm::vec2{0.0F, -25.0F}); break;
        case SDLK_DOWN: camera_.setCenter(camera_.center() + glm::vec2{0.0F, 25.0F}); break;
        case SDLK_LEFT: camera_.setCenter(camera_.center() + glm::vec2{-25.0F, 0.0F}); break;
        case SDLK_RIGHT: camera_.setCenter(camera_.center() + glm::vec2{25.0F, 0.0F}); break;

        case SDLK_K: camera_.setZoom(camera_.zoom() * 1.1F); break;
        case SDLK_J: camera_.setZoom(camera_.zoom() / 1.1F); break;

        case SDLK_R: particleSystem_.randomize(); break;
        default: break;
    }
}

void App::handleMouseMotion(const SDL_MouseMotionEvent& motion) {
    mousePosition_ = glm::vec2{motion.x, motion.y};
}
}  // namespace ps::app
