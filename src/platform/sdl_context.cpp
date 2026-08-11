#include "platform/sdl_context.hpp"

#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace ps::platform {

SdlContext::SdlContext() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        const int version = SDL_GetVersion();
        const int major = SDL_VERSIONNUM_MAJOR(version);
        const int minor = SDL_VERSIONNUM_MINOR(version);
        const int micro = SDL_VERSIONNUM_MICRO(version);

        throw std::runtime_error{
            std::string{"Failed to initialize SDL v"} + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(micro) + ": " +
            SDL_GetError()
        };
    }
}

SdlContext::~SdlContext() {
    SDL_Quit();
}

}  // namespace ps::platform
