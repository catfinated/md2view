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

    static constexpr float kYaw = -90.0f; ///< Default yaw (faces -Z).
    static constexpr float kPitch = 0.0f; ///< Default pitch (level).
    static constexpr float kFov = 45.0f;  ///< Default field of view in degrees.

    /// @param position World-space starting position.
    /// @param up       World up vector.
    /// @param yaw      Initial yaw in degrees.
    /// @param pitch    Initial pitch in degrees.
    explicit Camera(glm::vec3 const& position = glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3 const& up = glm::vec3(0.0f, 1.0f, 0.0f),
                    float yaw = kYaw,
                    float pitch = kPitch);

    /// Compute the view matrix from the current position and orientation.
    [[nodiscard]] glm::mat4 viewMatrix() const;

    /// Current field of view in degrees.
    [[nodiscard]] float fov() const { return fov_; }

    /// World-space camera position.
    [[nodiscard]] glm::vec3 const& position() const { return position_; }
    /// Unit vector pointing in the direction the camera faces.
    [[nodiscard]] glm::vec3 const& front() const { return front_; }
    /// Camera-local up vector (recomputed each time orientation changes).
    [[nodiscard]] glm::vec3 const& up() const { return up_; }
    /// Camera-local right vector (recomputed each time orientation changes).
    [[nodiscard]] glm::vec3 const& right() const { return right_; }
    /// Current pitch in degrees. Clamped to [-89, 89] when `constrain_pitch` is
    /// set.
    [[nodiscard]] float pitch() const { return pitch_; }
    /// Current yaw in degrees. Increases rightward.
    [[nodiscard]] float yaw() const { return yaw_; }

    /// Set the field of view in degrees and mark the projection dirty.
    void setFov(float fov) {
        fov_ = fov;
        fovDirty_ = true;
    }

    /// Move the camera in a cardinal direction scaled by @p delta_time.
    void move(Direction direction, float deltaTime);

    /// Update yaw and pitch from a mouse delta.
    ///
    /// @param xoffset        Horizontal mouse delta in pixels.
    /// @param yoffset        Vertical mouse delta in pixels.
    /// @param constrain_pitch Clamp pitch to ±89° to prevent gimbal flip.
    void
    onMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    /// Adjust field of view from a scroll wheel delta.
    void onMouseScroll(double xoffset, double yoffset);

    /// Reset position, orientation, and FOV to defaults.
    /// @param position New world-space position.
    void reset(glm::vec3 const& position);

    void setPosition(glm::vec3 const& pos) { position_ = pos; }

    /// True if the view matrix needs to be recomputed this frame.
    [[nodiscard]] bool viewDirty() const { return viewDirty_; }
    void setViewClean() { viewDirty_ = false; }

    /// True if the projection matrix needs to be recomputed this frame.
    [[nodiscard]] bool fovDirty() const { return fovDirty_; }
    void setFovClean() { fovDirty_ = false; }
    void setFovDirty() { fovDirty_ = true; }

private:
    void updateVectors();

    glm::vec3 position_;
    glm::vec3 front_;
    glm::vec3 up_;
    glm::vec3 right_;
    glm::vec3 worldUp_;

    float yaw_;
    float pitch_;
    float movementSpeed_{3.0f};
    float mouseSensitivity_{0.25};
    float fov_{kFov};

    bool viewDirty_ = true;
    bool fovDirty_ = true;
};
