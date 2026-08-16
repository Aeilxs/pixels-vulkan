#pragma once

struct SDL_Window;

namespace ps::platform {

/// @brief Owns an SDL window configured for Vulkan presentation.
///
/// The window is resizable, supports high-density displays and is destroyed
/// automatically with the owning object. It is intentionally neither copyable
/// nor movable to keep ownership of the native SDL handle unambiguous.
class Window final {
   public:
    Window(const char* title, int width, int height);

    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    struct DrawableSize {
        int width = 0;
        int height = 0;
    };

    struct LogicalSize {
        int width = 0;
        int height = 0;
    };

    [[nodiscard("Drawable size must be used")]]
    DrawableSize drawableSize() const;

    [[nodiscard("Logical size must be used")]]
    LogicalSize logicalSize() const;

    [[nodiscard("SDL window handle must be used")]]
    SDL_Window* nativeHandle() const noexcept;

   private:
    SDL_Window* handle_{nullptr};
};

}  // namespace ps::platform
