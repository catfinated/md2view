#pragma once

#include "common.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <vector>

class Camera
{
public:
    enum Direction {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    static constexpr float YAW =  -90.0f;
    static constexpr float PITCH = 0.0f;
    static constexpr float SPEED = 3.0f;
    static constexpr float SENSITIVITY = 0.25f;
    static constexpr float ZOOM = 45.0f;

    Camera(glm::vec3 const& position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 const& up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH);

    glm::mat4 view_matrix() const;
    float zoom() const { return zoom_; }

    void move(Direction direction, float delta_time);
    void process_mouse_movement(float xoffset, float yoffset, bool constrain_pitch = true);
    void process_mouse_scroll(float yoffset);
    void reset(glm::vec3 const&);
    void set_position(glm::vec3 const& pos) { position_ = pos; }
    void draw_ui();

private:
    void update_vectors();

private:
    glm::vec3 position_;
    glm::vec3 front_;
    glm::vec3 up_;
    glm::vec3 right_;
    glm::vec3 world_up_;

    float yaw_;
    float pitch_;
    float movement_speed_;
    float mouse_sensitivity_;
    float zoom_;
};

inline Camera::Camera(glm::vec3 const& position,
                      glm::vec3 const& up,
                      float yaw,
                      float pitch)
    : position_(position)
    , front_(0.0f, 0.0f, -1.0f)
    , world_up_(up)
    , yaw_(yaw)
    , pitch_(pitch)
    , movement_speed_(SPEED)
    , mouse_sensitivity_(SENSITIVITY)
    , zoom_(ZOOM)
{
    update_vectors();
}

inline void Camera::reset(glm::vec3 const& position)
{
    position_ = position;
    front_ = glm::vec3(0.0f, 0.0f, -1.0f);
    world_up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    yaw_ = YAW;
    pitch_ = PITCH;
    movement_speed_ = SPEED;
    mouse_sensitivity_ = SENSITIVITY;
    zoom_ = ZOOM;

    update_vectors();
}

inline glm::mat4 Camera::view_matrix() const
{
    return glm::lookAt(position_, position_ + front_, up_);
}

inline void Camera::move(Direction direction, float delta_time)
{
    float velocity = movement_speed_ * delta_time;

    switch (direction) {
    case FORWARD:
        position_ += front_ * velocity;
        break;
    case BACKWARD:
        position_ -= front_ * velocity;
        break;
    case LEFT:
        position_ -= right_ * velocity;
        break;
    case RIGHT:
        position_ += right_ * velocity;
        break;
    default:
        break;
    }
}

inline void Camera::process_mouse_movement(float xoffset, float yoffset, bool constrain_pitch)
{
    xoffset *= mouse_sensitivity_;
    yoffset *= mouse_sensitivity_;

    yaw_ += xoffset;
    pitch_ += yoffset;

    if (constrain_pitch) {
        pitch_ = std::min(89.0f, std::max(-89.0f, pitch_));
    }

    update_vectors();
}

inline void Camera::process_mouse_scroll(float yoffset)
{
    if (zoom_ >= 1.0f && zoom_ <= 45.0f) {
        zoom_ -= yoffset;
    }

    zoom_ = std::min(45.0f, std::max(1.0f, zoom_));
}

inline void Camera::update_vectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(pitch_)) * cos(glm::radians(yaw_));
    front.y = sin(glm::radians(pitch_));
    front.z = cos(glm::radians(pitch_)) * sin(glm::radians(yaw_));
    front_ = glm::normalize(front);
    right_ = glm::normalize(glm::cross(front_, world_up_));
    up_ = glm::normalize(glm::cross(right_, front_));
}

inline void Camera::draw_ui()
{
    ImGui::InputFloat3("Position", glm::value_ptr(position_), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Front", glm::value_ptr(front_), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Up", glm::value_ptr(up_), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Right", glm::value_ptr(right_), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::SliderFloat("fov", &zoom_, 1.0f, 45.0f);
}
