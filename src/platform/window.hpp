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
    /// @brief Creates a Vulkan-capable SDL window.
    /// @param title Null-terminated window title.
    /// @param width Initial width in logical pixels.
    /// @param height Initial height in logical pixels.
    /// @throws std::runtime_error If SDL cannot create the window.
    Window(const char* title, int width, int height);

    /// @brief Destroys the owned SDL window.
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    /// @brief Physical drawable size of a window, expressed in pixels.
    struct DrawableSize {
        int width = 0;
        int height = 0;
    };

    /// @brief Returns the current drawable size of the window in physical pixels.
    /// @return Current drawable width and height.
    /// @throws std::runtime_error If SDL cannot query the window size.
    [[nodiscard("Drawable size must be used")]]
    DrawableSize drawableSize() const;

    /// @brief Returns the native SDL window handle without transferring ownership.
    /// @return The owned SDL window handle, valid for this object's lifetime.
    [[nodiscard("SDL window handle must be used")]]
    SDL_Window* nativeHandle() const noexcept;

   private:
    SDL_Window* handle_{nullptr};
};

}  // namespace ps::platform
