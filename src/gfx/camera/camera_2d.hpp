#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace ps::gfx {

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
