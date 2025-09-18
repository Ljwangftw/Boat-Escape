#include "Camera.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Position(position), Front(glm::vec3(0.0f, 0.0f, -1.0f)), WorldUp(up),
      Yaw(yaw), Pitch(pitch), mode(THIRD_PERSON) {
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix(const glm::vec3& shipPos,
                                const glm::vec3& shipFront,
                                float shipYawRadians) {
    if (mode == FIRST_PERSON) {
        glm::mat4 shipTransform = glm::mat4(1.0f);
        shipTransform = glm::translate(shipTransform, shipPos);
        shipTransform = glm::rotate(shipTransform, shipYawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        shipTransform = glm::scale(shipTransform, glm::vec3(0.4f));
        glm::vec4 shipCenter = shipTransform * glm::vec4(1.0f, 0.0f, -0.875f, 1.0f);
        glm::vec3 actualShipCenter = glm::vec3(shipCenter);

        glm::vec3 modelUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 cameraPos = actualShipCenter + shipFront * (-0.3f) + modelUp * 0.6f;

        glm::vec3 front_with_pitch = shipFront;
        front_with_pitch.y = sin(glm::radians(Pitch));
        float horizontal_scale = cos(glm::radians(Pitch));
        front_with_pitch.x *= horizontal_scale;
        front_with_pitch.z *= horizontal_scale;

        Position = cameraPos;
        return glm::lookAt(cameraPos,
                           cameraPos + glm::normalize(front_with_pitch),
                           modelUp);
    }

    glm::vec3 cameraPos = shipPos - Front * 7.0f + glm::vec3(0.0f, 0.4f, 0.0f);
    Position = cameraPos;
    return glm::lookAt(cameraPos, shipPos, WorldUp);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= 0.1f;
    yoffset *= 0.1f;

    Yaw   += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::SetYaw(float yawDegrees) {
    Yaw = yawDegrees;
    updateCameraVectors();
}

void Camera::SetPitch(float pitchDegrees) {
    Pitch = pitchDegrees;
    updateCameraVectors();
}

void Camera::RecalculateVectors() {
    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}
