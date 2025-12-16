#pragma once 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace kcShaders {

class Camera {

public:
    Camera(float fov = 45.0f, float aspect_ratio = 4.0f/3.0f, float near_plane = 0.1f, float far_plane = 100.0f);
    ~Camera();

    void SetPosition(const glm::vec3& position);
    void SetTarget(const glm::vec3& target);
    void SetUpVector(const glm::vec3& up);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    void ProcessMouseMovement(float xoffset, float yoffset);
    void ProcessMouseScroll(float yoffset);

private:
    glm::vec3 position_;
    glm::vec3 target_;
    glm::vec3 up_;
    glm::vec3 front_;
    glm::vec3 right_;

    float yaw_;
    float pitch_;
    float fov_;
    float aspect_ratio_;
    float near_plane_;
    float far_plane_;

    void UpdateCameraVectors();
};

} // namespace kcShaders