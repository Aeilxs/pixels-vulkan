#include "gfx/camera/camera_2d.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <stdexcept>

namespace ps::gfx {

Camera2D::Camera2D(glm::vec2 viewSize, float zoom) {
    setViewSize(viewSize);
    setZoom(zoom);
}

void Camera2D::setCenter(glm::vec2 center) noexcept {
    center_ = center;
}

void Camera2D::setZoom(float zoom) {
    if (zoom <= 0.0F) {
        throw std::invalid_argument("Camera zoom must be greater than 0.");
    }

    zoom_ = zoom;
}

void Camera2D::setViewSize(glm::vec2 viewSize) {
    if (viewSize.x <= 0.0F || viewSize.y <= 0.0F) {
        throw std::invalid_argument("Camera view size must be greater than 0.");
    }

    viewSize_ = viewSize;
}

void Camera2D::fit(glm::vec2 contentCenter, glm::vec2 contentSize, float fill) {
    if (contentSize.x <= 0.0F || contentSize.y <= 0.0F) {
        throw std::invalid_argument("Content size must be greater than 0.");
    }
    if (fill <= 0.0F || fill > 1.0F) {
        throw std::invalid_argument("Fill factor must be in the range (0, 1].");
    }

    const float horizontalZoom = viewSize_.x / contentSize.x;
    const float verticalZoom = viewSize_.y / contentSize.y;

    setCenter(contentCenter);
    setZoom(std::min(horizontalZoom, verticalZoom) * fill);
}

glm::vec2 Camera2D::center() const noexcept {
    return center_;
}

float Camera2D::zoom() const noexcept {
    return zoom_;
}

glm::vec2 Camera2D::viewSize() const noexcept {
    return viewSize_;
}

glm::mat4 Camera2D::viewProjection() const noexcept {
    const glm::vec2 halfView = viewSize_ / (2.0F * zoom_);

    const glm::mat4 projection = glm::ortho(
        -halfView.x,
        halfView.x,
        -halfView.y,
        halfView.y,
        -1.0F,
        1.0F
    );

    const glm::mat4 view = glm::translate(glm::mat4{1.0F}, glm::vec3{-center_, 0.0F});

    return projection * view;
}

glm::vec2 Camera2D::screenToWorld(glm::vec2 screenPosition) const noexcept {
    return center_ + (screenPosition - viewSize_ * 0.5F) / zoom_;
}

}  // namespace ps::gfx
