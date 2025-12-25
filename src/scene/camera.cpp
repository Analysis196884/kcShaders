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
    
    // Recalculate yaw and pitch based on the new target
    glm::vec3 direction = target_ - position_;
    direction = glm::normalize(direction);
    
    // Calculate yaw
    yaw_ = glm::degrees(atan2(direction.z, direction.x));
    
    // Calculate pitch
    float horizontalDistance = sqrt(direction.x * direction.x + direction.z * direction.z);
    pitch_ = glm::degrees(atan2(direction.y, horizontalDistance));
    
    // Constrain pitch
    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;
    
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
    RotateView(xoffset, yoffset);
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

void Camera::MoveForward(float distance)
{
    position_ += front_ * distance;
    target_ = position_ + front_;
}

void Camera::MoveBackward(float distance)
{
    position_ -= front_ * distance;
    target_ = position_ + front_;
}

void Camera::MoveLeft(float distance)
{
    position_ -= right_ * distance;
    target_ = position_ + front_;
}

void Camera::MoveRight(float distance)
{
    position_ += right_ * distance;
    target_ = position_ + front_;
}

void Camera::MoveUp(float distance)
{
    position_ += up_ * distance;
    target_ = position_ + front_;
}

void Camera::MoveDown(float distance)
{
    position_ -= up_ * distance;
    target_ = position_ + front_;
}

void Camera::RotateView(float yawDelta, float pitchDelta)
{
    yaw_ += yawDelta;
    pitch_ += pitchDelta;

    // Constrain pitch
    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;

    UpdateCameraVectors();
    target_ = position_ + front_;
}

} // namespace kcShaders