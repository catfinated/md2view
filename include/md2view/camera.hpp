#pragma once

#include <glm/glm.hpp>

#include <cstdint>

/// First-person fly camera with mouse-look and scroll-to-zoom.
///
/// Tracks position, yaw, pitch, and field of view. View and projection
/// state is lazily recomputed: callers should check `view_dirty()` and
/// `fov_dirty()` each frame and only rebuild matrices when the flags are set,
/// then call the corresponding `set_*_clean()` to clear them.
///
/// @note Based on the camera tutorial at
/// https://learnopengl.com/Getting-started/Camera
class Camera {
public:
    /// Movement direction for `move()`.
    enum class Direction : std::uint8_t { FORWARD, BACKWARD, LEFT, RIGHT };

    static constexpr float YAW = -90.0f; ///< Default yaw (faces -Z).
    static constexpr float PITCH = 0.0f; ///< Default pitch (level).
    static constexpr float FOV = 45.0f;  ///< Default field of view in degrees.

    /// @param position World-space starting position.
    /// @param up       World up vector.
    /// @param yaw      Initial yaw in degrees.
    /// @param pitch    Initial pitch in degrees.
    Camera(glm::vec3 const& position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 const& up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH);

    /// Compute the view matrix from the current position and orientation.
    [[nodiscard]] glm::mat4 view_matrix() const;

    /// Current field of view in degrees.
    [[nodiscard]] float fov() const { return fov_; }

    /// Move the camera in a cardinal direction scaled by @p delta_time.
    void move(Direction direction, float delta_time);

    /// Update yaw and pitch from a mouse delta.
    ///
    /// @param xoffset        Horizontal mouse delta in pixels.
    /// @param yoffset        Vertical mouse delta in pixels.
    /// @param constrain_pitch Clamp pitch to ±89° to prevent gimbal flip.
    void on_mouse_movement(float xoffset,
                           float yoffset,
                           bool constrain_pitch = true);

    /// Adjust field of view from a scroll wheel delta.
    void on_mouse_scroll(double xoffset, double yoffset);

    /// Reset position, orientation, and FOV to defaults.
    /// @param position New world-space position.
    void reset(glm::vec3 const& position);

    void set_position(glm::vec3 const& pos) { position_ = pos; }

    /// Draw an ImGui panel exposing camera parameters.
    void draw_ui();

    /// True if the view matrix needs to be recomputed this frame.
    [[nodiscard]] bool view_dirty() const { return view_dirty_; }
    void set_view_clean() { view_dirty_ = false; }

    /// True if the projection matrix needs to be recomputed this frame.
    [[nodiscard]] bool fov_dirty() const { return fov_dirty_; }
    void set_fov_clean() { fov_dirty_ = false; }

private:
    void update_vectors();

    glm::vec3 position_;
    glm::vec3 front_;
    glm::vec3 up_;
    glm::vec3 right_;
    glm::vec3 world_up_;

    float yaw_;
    float pitch_;
    float movement_speed_{3.0f};
    float mouse_sensitivity_{0.25};
    float fov_{FOV};

    bool view_dirty_ = true;
    bool fov_dirty_ = true;
};
