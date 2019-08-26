#pragma once

#include <GL/glew.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <vector>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

GLfloat const YAW = -90.0f;
GLfloat const PITCH = 0.0f;
GLfloat const SPEED = 3.0f;
GLfloat const SENSITIVITY = 0.25f;
GLfloat const ZOOM = 45.0f;

class Camera
{
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    GLfloat Yaw;
    GLfloat Pitch;
    GLfloat MovementSpeed;
    GLfloat MouseSensitivity;
    GLfloat Zoom;

    Camera(glm::vec3 const& position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 const& up = glm::vec3(0.0f, 1.0f, 0.0f),
           GLfloat yaw = YAW,
           GLfloat pitch = PITCH);

    Camera(GLfloat posX, GLfloat posY, GLfloat posZ,
           GLfloat upX,  GLfloat upY,  GLfloat upZ,
           GLfloat yaw,  GLfloat pitch);

    glm::mat4 GetViewMatrix() const;
    void ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime);
    void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constrainPitch = true);
    void ProcessMouseScroll(GLfloat yoffset);

    void reset();

private:
    void updateCameraVectors();
};

inline
Camera::Camera(glm::vec3 const& position,
               glm::vec3 const& up,
               GLfloat yaw,
               GLfloat pitch)
    : Position(position)
    , Front(0.0f, 0.0f, -1.0f)
    , WorldUp(up)
    , Yaw(yaw)
    , Pitch(pitch)
    , MovementSpeed(SPEED)
    , MouseSensitivity(SENSITIVITY)
    , Zoom(ZOOM)
{
    updateCameraVectors();
}

inline
Camera::Camera(GLfloat posX, GLfloat posY, GLfloat posZ,
               GLfloat upX,  GLfloat upY,  GLfloat upZ,
               GLfloat yaw,  GLfloat pitch)
    : Position(posX, posY, posZ)
    , Front(0.0f, 0.0f, -1.0f)
    , WorldUp(upX, upY, upZ)
    , Yaw(yaw)
    , Pitch(pitch)
    , MovementSpeed(SPEED)
    , MouseSensitivity(SENSITIVITY)
    , Zoom(ZOOM)
{
    updateCameraVectors();
}

inline
void Camera::reset()
{
    Position = glm::vec3(0.0f, 0.0f, 0.0f);
    Front = glm::vec3(0.0f, 0.0f, -1.0f);
    WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    Yaw = YAW;
    Pitch = PITCH;
    MovementSpeed = SPEED;
    MouseSensitivity = SENSITIVITY;
    Zoom = ZOOM;
    updateCameraVectors();
}

inline
glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

inline
void Camera::ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
{
    GLfloat velocity = MovementSpeed * deltaTime;

    switch (direction) {
    case FORWARD:
        Position += Front * velocity;
        break;
    case BACKWARD:
        Position -= Front * velocity;
        break;
    case LEFT:
        Position -= Right * velocity;
        break;
    case RIGHT:
        Position += Right * velocity;
        break;
    default:
        break;
    }
}

inline
void Camera::ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        Pitch = std::min(89.0f, std::max(-89.0f, Pitch));
    }

    updateCameraVectors();
}

inline
void Camera::ProcessMouseScroll(GLfloat yoffset)
{
    if (Zoom >= 1.0f && Zoom <= 45.0f) {
        Zoom -= yoffset;
    }

    Zoom = std::min(45.0f, std::max(1.0f, Zoom));
}

inline
void Camera::updateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
    front.y = sin(glm::radians(Pitch));
    front.z = cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
