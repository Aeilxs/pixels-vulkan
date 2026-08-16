#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace ps::gfx {

/// @brief Represents a simple orthographic 2D camera in logical screen units.
///
/// World coordinates use +X to the right and +Y downward. The camera stores a world-space
/// center, a logical view size and a positive zoom factor, then exposes the world-to-clip
/// transform consumed by the renderer.
class Camera2D {
   public:
    explicit Camera2D(glm::vec2 viewSize, float zoom = 1.0F);

    void setCenter(glm::vec2 center) noexcept;

    void setZoom(float zoom);

    void setViewSize(glm::vec2 viewSize);

    void fit(glm::vec2 contentCenter, glm::vec2 contentSize, float fill = 0.9F);

    [[nodiscard]]
    glm::vec2 center() const noexcept;

    [[nodiscard]]
    float zoom() const noexcept;

    [[nodiscard]]
    glm::vec2 viewSize() const noexcept;

    [[nodiscard]]
    glm::mat4 viewProjection() const noexcept;

    [[nodiscard]]
    glm::vec2 screenToWorld(glm::vec2 screenPosition) const noexcept;

   private:
    glm::vec2 viewSize_{};
    glm::vec2 center_{};
    float zoom_{1.0F};
};

}  // namespace ps::gfx
