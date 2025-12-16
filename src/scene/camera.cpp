#include "camera.h"

namespace kcShaders {

Camera::Camera(float fov, float aspect_ratio, float near_plane, float far_plane)
    : position_(glm::vec3(0.0f, 0.0f, 3.0f))
    , target_(glm::vec3(0.0f, 0.0f, 0.0f))
    , up_(glm::vec3(0.0f, 1.0f, 0.0f))
    , yaw_(-90.0f)
    , pitch_(0.0f)
    , fov_(fov)
    , aspect_ratio_(aspect_ratio)
    , near_plane_(near_plane)
    , far_plane_(far_plane)
{
    UpdateCameraVectors();
}

Camera::~Camera()
{
}

void Camera::SetPosition(const glm::vec3& position)
{
    position_ = position;
    UpdateCameraVectors();
}

void Camera::SetTarget(const glm::vec3& target)
{
    target_ = target;
    UpdateCameraVectors();
}

void Camera::SetUpVector(const glm::vec3& up)
{
    up_ = up;
    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(position_, target_, up_);
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    return glm::perspective(glm::radians(fov_), aspect_ratio_, near_plane_, far_plane_);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset)
{
    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw_ += xoffset;
    pitch_ += yoffset;

    // Constrain pitch
    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;

    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    fov_ -= yoffset;
    if (fov_ < 1.0f) fov_ = 1.0f;
    if (fov_ > 45.0f) fov_ = 45.0f;
}

void Camera::UpdateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front.y = sin(glm::radians(pitch_));
    front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_ = glm::normalize(front);
    right_ = glm::normalize(glm::cross(front_, up_));
}

} // namespace kcShaders