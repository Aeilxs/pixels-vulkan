#include "app/app.hpp"
#include "app/config.hpp"
#include "app/diagnostics.hpp"
#include "gfx/fonts/font_atlas.hpp"
#include "log/log.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

namespace {
// Use the actual logical window size; the window manager may adjust the size requested at creation.
glm::vec2 windowLogicalSize(const ps::platform::Window& window) {
    const ps::platform::Window::LogicalSize size = window.logicalSize();

    return {
        static_cast<float>(size.width),
        static_cast<float>(size.height),
    };
}

std::string formatOverlay(const ps::app::FrameMetrics& metrics, std::size_t particleCount) {
    std::ostringstream stream;
    stream << std::fixed;
    stream << "Particles Count " << std::setw(8) << particleCount << '\n';
    stream << "FPS             " << std::setw(8) << std::setprecision(1) << metrics.fps << '\n';
    stream << "Frame           " << std::setw(8) << std::setprecision(1) << metrics.frameWallTimeMs << " ms\n";
    stream << "Simulation      " << std::setw(8) << metrics.simulationTimeMs << " ms\n";
    stream << "Upload          " << std::setw(8) << metrics.particleUploadTimeMs << " ms\n";
    stream << "Fence           " << std::setw(8) << metrics.fenceWaitTimeMs << " ms";
    return stream.str();
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
      swapchain_{physicalDevice_, device_, vulkanSurface_, window_, options.uncapped},
      renderer_{
          physicalDevice_,
          device_,
          swapchain_,
          particleSystem_.particles(),
          gfx::fonts::FontAtlas::fromTrueType(config::overlayFontPath, config::overlayFontPixelHeight)
      },
      benchmarkOutputPath_{options.benchmark_output_path} {
    const glm::vec2 contentSize{
        static_cast<float>(particleSystem_.imageDimensions().width),
        static_cast<float>(particleSystem_.imageDimensions().height),
    };

    diagnostics::logVulkanRuntime(physicalDevice_, swapchain_);

    ps::log::trace("Run with image: %s, gap: %d", options.image_path.string().c_str(), options.gap);
    ps::log::trace("Particle system content size: %.1f x %.1f", contentSize.x, contentSize.y);
    ps::log::trace("Particle system particle count: %zu", particleSystem_.particles().size());

    camera_.fit(contentSize * 0.5F, contentSize, 0.95F);
    renderer_.setOverlayText("Collecting frame metrics...");
}

void App::run() {
    const auto benchmarkStartTime = std::chrono::steady_clock::now();
    auto previousFrameStartTime = benchmarkStartTime;
    // A frame's complete start-to-start wall time is only known when the next frame begins.
    std::optional<FrameSample> pendingFrameSample{};

    while (running_) {
        const auto frameStartTime = std::chrono::steady_clock::now();
        const auto frameWallTime = frameStartTime - previousFrameStartTime;
        previousFrameStartTime = frameStartTime;

        if (pendingFrameSample.has_value()) {
            pendingFrameSample->frameWallTime = frameWallTime;

            if (frameStats_.push(*pendingFrameSample)) {
                const auto& metrics = frameStats_.metrics();
                const std::size_t particleCount = particleSystem_.particles().size();

                renderer_.setOverlayText(formatOverlay(metrics, particleCount));
                ps::log::trace(
                    "Frame metrics | particles=%zu fps=%.1f frame=%.3fms simulation=%.3fms upload=%.3fms fence=%.3fms",
                    particleCount,
                    metrics.fps,
                    metrics.frameWallTimeMs,
                    metrics.simulationTimeMs,
                    metrics.particleUploadTimeMs,
                    metrics.fenceWaitTimeMs
                );

                if (!benchmarkOutputPath_.empty()) {
                    benchmarkSamples_.push_back(BenchmarkSample{
                        .elapsedTimeSeconds = std::chrono::duration<double>(frameStartTime - benchmarkStartTime).count(),
                        .particleCount = particleCount,
                        .metrics = metrics,
                    });
                }
            }
        }

        const float dt = std::min(std::chrono::duration<float>(frameWallTime).count(), 0.05F);

        pollEvents();
        if (!running_) {
            break;
        }

        std::optional<glm::vec2> mouseWorldPosition{};
        if (mousePosition_.has_value()) {
            mouseWorldPosition = camera_.screenToWorld(*mousePosition_);
        }

        const auto simulationStartTime = std::chrono::steady_clock::now();
        particleSystem_.update(dt, mouseWorldPosition);
        const auto simulationTime = std::chrono::steady_clock::now() - simulationStartTime;

        const ps::vulkan::FrameTimings rendererTimings = renderer_.drawFrame(camera_.viewProjection(), particleSystem_.particles());
        pendingFrameSample = FrameSample{
            .simulationTime = simulationTime,
            .particleUploadTime = rendererTimings.particleUploadTime,
            .fenceWaitTime = rendererTimings.fenceWaitTime,
        };
    }

    if (!benchmarkOutputPath_.empty()) {
        writeBenchmarkCsv(benchmarkOutputPath_, benchmarkSamples_);
        ps::log::info(
            "Benchmark CSV written to %s (%zu samples)",
            benchmarkOutputPath_.string().c_str(),
            benchmarkSamples_.size()
        );
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
