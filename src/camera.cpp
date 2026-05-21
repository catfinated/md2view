#include "md2view/camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gsl-lite/gsl-lite.hpp>

Camera::Camera(glm::vec3 const& position,
               glm::vec3 const& up,
               float yaw,
               float pitch)
    : position_(position)
    , front_(0.0f, 0.0f, -1.0f)
    , up_(up)
    , right_(1.0f, 0.0f, 0.0)
    , world_up_(up)
    , yaw_(yaw)
    , pitch_(pitch) {
    update_vectors();
}

void Camera::reset(glm::vec3 const& position) {
    position_ = position;
    front_ = glm::vec3(0.0f, 0.0f, -1.0f);
    world_up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    yaw_ = YAW;
    pitch_ = PITCH;
    fov_ = FOV;

    update_vectors();
}

glm::mat4 Camera::view_matrix() const {
    return glm::lookAt(position_, position_ + front_, up_);
}

void Camera::move(Direction direction, float delta_time) {
    float velocity = movement_speed_ * delta_time;

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

    view_dirty_ = true;
}

void Camera::on_mouse_movement(float xoffset,
                               float yoffset,
                               bool constrain_pitch) {
    xoffset *= mouse_sensitivity_;
    yoffset *= mouse_sensitivity_;

    yaw_ += xoffset;
    pitch_ += yoffset;

    if (constrain_pitch) {
        pitch_ = std::min(89.0f, std::max(-89.0f, pitch_));
    }

    update_vectors();
}

void Camera::on_mouse_scroll(double /* xoffset */, double yoffset) {
    if (fov_ >= 1.0f && fov_ <= 45.0f) {
        fov_ -= gsl_lite::narrow_cast<float>(yoffset);
    }

    fov_ = std::min(45.0f, std::max(1.0f, fov_));
    fov_dirty_ = true;
}

void Camera::update_vectors() {
    auto const x =
        glm::cos(glm::radians(pitch_)) * glm::cos(glm::radians(yaw_));
    auto const y = glm::sin(glm::radians(pitch_));
    auto const z =
        glm::cos(glm::radians(pitch_)) * glm::sin(glm::radians(yaw_));
    front_ = glm::normalize(glm::vec3(x, y, z));
    right_ = glm::normalize(glm::cross(front_, world_up_));
    up_ = glm::normalize(glm::cross(right_, front_));
    view_dirty_ = true;
}
