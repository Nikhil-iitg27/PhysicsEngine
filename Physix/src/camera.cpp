#include "camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp> // for length2

Camera::Camera(const glm::vec3& cameraPos, const glm::vec3& cameraDir, const int mode) {
    this->Mode = mode;

    this->Position = cameraPos;
    this->Direction = glm::normalize(cameraDir);

    this->ProjectionAngle = 45.0f;
    this->FocusLength = glm::length(cameraDir);

    this->Speed = 10.0f;
    this->AngularSpeed = 45.0f;

    this->MinDist = 0.1f;
    this->MaxDist = 500.0f;

    //Calculate pitch (vertical angle) from direction
    glm::vec3 flatDir = glm::normalize(glm::vec3(this->Direction.x, 0.0f, this->Direction.z));

    if (glm::length2(flatDir) < 1e-6f) {
        // Looking straight up/down pitch
        this->Pitch = (this->Direction.y > 0.0f) ? 80.0f : -80.0f;
    }
    else {
        float angle = glm::degrees(glm::angle(this->Direction, flatDir));
        float rawPitch = (this->Direction.y > 0.0f) ? angle : -angle;
        this->Pitch = glm::clamp(rawPitch, -80.0f, 80.0f);
    }
}


void Camera::detectKeyPress(GLFWwindow* window, float deltaTime) {
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(this->Direction, up));

    float moveStep = this->Speed * deltaTime;
    float rotateStep = glm::radians(this->AngularSpeed * deltaTime);

    // Movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        this->Position += this->Direction * moveStep;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        this->Position -= this->Direction * moveStep;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        this->Position -= right * moveStep;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        this->Position += right * moveStep;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        this->Position.y += moveStep;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        this->Position.y -= moveStep;

    // Rotation - Yaw (left/right)
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        this->Direction = glm::normalize(glm::rotate(this->Direction, rotateStep, up));
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        this->Direction = glm::normalize(glm::rotate(this->Direction, -rotateStep, up));

    // Rotation - Pitch (up/down), clamped to ±80 degrees
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && this->Pitch < 80.0f) {
        this->Direction = glm::normalize(glm::rotate(this->Direction, rotateStep, right));
        this->Pitch += glm::degrees(rotateStep);
        if (this->Pitch > 80.0f) this->Pitch = 80.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && this->Pitch > -80.0f) {
        this->Direction = glm::normalize(glm::rotate(this->Direction, -rotateStep, right));
        this->Pitch -= glm::degrees(rotateStep);
        if (this->Pitch < -80.0f) this->Pitch = -80.0f;
    }
}


float Camera::distanceFromCameraPlane(const glm::vec3& point) const {
    return glm::dot(point - this->Position, this->Direction);
}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 target = this->Position + this->Direction * this->FocusLength;
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    return glm::lookAt(this->Position, target, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(this->ProjectionAngle), aspectRatio, this->MinDist, this->MaxDist);
}
