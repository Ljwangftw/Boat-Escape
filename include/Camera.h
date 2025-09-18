#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    enum Mode {
        FIRST_PERSON,
        THIRD_PERSON
    };

    Camera(glm::vec3 position = glm::vec3(0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = 90.0f,
           float pitch = 0.0f);

    glm::mat4 GetViewMatrix(const glm::vec3& shipPos,
                            const glm::vec3& shipFront,
                            float shipYawRadians);

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    void SetYaw(float yawDegrees);
    void SetPitch(float pitchDegrees);
    void RecalculateVectors();

public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    Mode  mode;

private:
    void updateCameraVectors();
};

#endif // CAMERA_H
