#pragma once

#include <glm/glm.hpp>

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
