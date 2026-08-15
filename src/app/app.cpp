#include "app/app.hpp"
#include "app/config.hpp"

#include <SDL3/SDL.h>

namespace ps::app {

App::App(const cli::AppOptions& options)
    : sdlContext_{},
      window_{config::applicationName, config::initialWindowWidth, config::initialWindowHeight},
      vulkanInstance_{},
      vulkanSurface_{vulkanInstance_, window_},
      physicalDevice_{vulkanInstance_, vulkanSurface_},
      device_{physicalDevice_},
      swapchain_{physicalDevice_, device_, vulkanSurface_, window_},
      renderer_{physicalDevice_, device_, swapchain_} {
}

void App::run() {
    while (running_) {
        pollEvents();

        if (running_) {
            renderer_.drawFrame();
        }
    }
}

void App::pollEvents() {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: running_ = false; break;
            default: break;
        }
    }
}

}  // namespace ps::app