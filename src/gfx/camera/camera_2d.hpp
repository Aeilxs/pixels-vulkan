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
    /// @brief Creates a camera for the given logical view size and zoom.
    /// @throws std::invalid_argument If the view size or zoom is not positive.
    explicit Camera2D(glm::vec2 viewSize, float zoom = 1.0F);

    /// @brief Sets the world-space point displayed at the center of the view.
    void setCenter(glm::vec2 center) noexcept;

    /// @brief Sets the zoom factor, where values greater than one zoom in.
    /// @throws std::invalid_argument If @p zoom is not positive.
    void setZoom(float zoom);

    /// @brief Sets the logical size used to build the orthographic projection.
    /// @throws std::invalid_argument If either component is not positive.
    void setViewSize(glm::vec2 viewSize);

    /// @brief Centers and scales the camera so a rectangular region fits in the view.
    /// @param contentCenter Center of the region in world coordinates.
    /// @param contentSize Width and height of the region in world units.
    /// @param fill Fraction of the available view to occupy, in the range (0, 1].
    /// @throws std::invalid_argument If the content size or fill factor is invalid.
    void fit(glm::vec2 contentCenter, glm::vec2 contentSize, float fill = 0.9F);

    /// @brief Returns the current world-space camera center.
    [[nodiscard]]
    glm::vec2 center() const noexcept;

    /// @brief Returns the current positive zoom factor.
    [[nodiscard]]
    float zoom() const noexcept;

    /// @brief Returns the logical view size used by the projection.
    [[nodiscard]]
    glm::vec2 viewSize() const noexcept;

    /// @brief Builds the matrix transforming world coordinates into renderer clip coordinates.
    [[nodiscard]]
    glm::mat4 viewProjection() const noexcept;

    /// @brief Converts logical screen coordinates into world coordinates.
    [[nodiscard]]
    glm::vec2 screenToWorld(glm::vec2 screenPosition) const noexcept;

   private:
    glm::vec2 viewSize_{};
    glm::vec2 center_{};
    float zoom_{1.0F};
};

}  // namespace ps::gfx
