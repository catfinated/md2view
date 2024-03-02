#pragma once

#include "common.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

// based on camera tutorial from https://learnopengl.com/Getting-started/Camera
class Camera
{
public:
    enum Direction {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    static constexpr float YAW = -90.0f;
    static constexpr float PITCH = 0.0f;
    static constexpr float FOV = 45.0f;

    Camera(glm::vec3 const& position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 const& up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH);

    glm::mat4 view_matrix() const;
    float fov() const { return fov_; }

    void move(Direction direction, float delta_time);
    void on_mouse_movement(float xoffset, float yoffset, bool constrain_pitch = true);
    void on_mouse_scroll(double xoffset, double yoffset);
    void reset(glm::vec3 const&);
    void set_position(glm::vec3 const& pos) { position_ = pos; }
    void draw_ui();

    bool view_dirty() { return view_dirty_; }
    void set_view_clean() { view_dirty_ = false; }

    bool fov_dirty() { return fov_dirty_; }
    void set_fov_clean() { fov_dirty_ = false; }

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
    float fov_;

    bool view_dirty_ = true;
    bool fov_dirty_ = true;
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
    , movement_speed_(3.0f)
    , mouse_sensitivity_(0.25)
    , fov_(FOV)
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
    fov_ = FOV;

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

    view_dirty_ = true;
}

inline void Camera::on_mouse_movement(float xoffset, float yoffset, bool constrain_pitch)
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

inline void Camera::on_mouse_scroll(double, double yoffset)
{
    if (fov_ >= 1.0f && fov_ <= 45.0f) {
        fov_ -= yoffset;
    }

    fov_ = std::min(45.0f, std::max(1.0f, fov_));
    fov_dirty_ = true;
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
    view_dirty_ = true;
}

inline void Camera::draw_ui()
{
    ImGui::InputFloat3("Position", glm::value_ptr(position_), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Front", glm::value_ptr(front_), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Up", glm::value_ptr(up_), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Right", glm::value_ptr(right_), "%.3f", ImGuiInputTextFlags_ReadOnly);
    if (ImGui::SliderFloat("fov", &fov_, 1.0f, 60.0f)) {
        fov_dirty_ = true;
    }
    ImGui::InputFloat("Pitch", &pitch_, 0.0, 0.0, "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat("Yaw", &yaw_, 0.0, 0.0, "%.3f", ImGuiInputTextFlags_ReadOnly);
}
