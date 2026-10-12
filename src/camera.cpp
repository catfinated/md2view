#include "md2view/camera.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <gsl-lite/gsl-lite.hpp>

#include <algorithm>

Camera::Camera(glm::vec3 const& position,
               glm::vec3 const& up,
               float yaw,
               float pitch)
    : position_(position)
    , front_(0.0f, 0.0f, -1.0f)
    , up_(up)
    , right_(1.0f, 0.0f, 0.0)
    , worldUp_(up)
    , yaw_(yaw)
    , pitch_(pitch) {
    updateVectors();
}

void Camera::reset(glm::vec3 const& position) {
    position_ = position;
    front_ = glm::vec3(0.0f, 0.0f, -1.0f);
    worldUp_ = glm::vec3(0.0f, 1.0f, 0.0f);
    yaw_ = kYaw;
    pitch_ = kPitch;
    fov_ = kFov;

    updateVectors();
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position_, position_ + front_, up_);
}

void Camera::move(Direction direction, float deltaTime) {
    float velocity = movementSpeed_ * deltaTime;

    switch (direction) {
    case Direction::FORWARD:
        position_ += front_ * velocity;
        break;
    case Direction::BACKWARD:
        position_ -= front_ * velocity;
        break;
    case Direction::LEFT:
        position_ -= right_ * velocity;
        break;
    case Direction::RIGHT:
        position_ += right_ * velocity;
        break;
    default:
        break;
    }

    viewDirty_ = true;
}

void Camera::onMouseMovement(float xoffset,
                             float yoffset,
                             bool constrainPitch) {
    xoffset *= mouseSensitivity_;
    yoffset *= mouseSensitivity_;

    yaw_ += xoffset;
    pitch_ += yoffset;

    if (constrainPitch) {
        pitch_ = std::min(89.0f, std::max(-89.0f, pitch_));
    }

    updateVectors();
}

void Camera::onMouseScroll(double /* xoffset */, double yoffset) {
    if (fov_ >= 1.0f && fov_ <= 45.0f) {
        fov_ -= gsl_lite::narrow_cast<float>(yoffset);
    }

    fov_ = std::min(45.0f, std::max(1.0f, fov_));
    fovDirty_ = true;
}

void Camera::updateVectors() {
    auto const x =
        glm::cos(glm::radians(pitch_)) * glm::cos(glm::radians(yaw_));
    auto const y = glm::sin(glm::radians(pitch_));
    auto const z =
        glm::cos(glm::radians(pitch_)) * glm::sin(glm::radians(yaw_));
    front_ = glm::normalize(glm::vec3(x, y, z));
    right_ = glm::normalize(glm::cross(front_, worldUp_));
    up_ = glm::normalize(glm::cross(right_, front_));
    viewDirty_ = true;
}
