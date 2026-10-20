// See camera.h for what Camera is and how its free-fly movement/look model
// works; this file is the implementation.
#include "camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp> // for length2
#include <GLFW/glfw3.h> // for GLFW_KEY_* constants used below

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


void Camera::Update(Input& input, float deltaTime) {
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(this->Direction, up));

    float moveStep = this->Speed * deltaTime;
    float rotateStep = glm::radians(this->AngularSpeed * deltaTime);

    // Mouse look (only while the cursor is captured -- see Input::SetCursorCaptured)
    if (input.IsCursorCaptured()) {
        const float mouseSensitivity = 0.15f; // degrees per pixel of mouse movement
        glm::vec2 mouseDelta = input.GetMouseDelta();

        if (mouseDelta.x != 0.0f) {
            float yaw = glm::radians(-mouseDelta.x * mouseSensitivity);
            this->Direction = glm::normalize(glm::rotate(this->Direction, yaw, up));
            right = glm::normalize(glm::cross(this->Direction, up));
        }
        if (mouseDelta.y != 0.0f) {
            float desiredPitch = glm::clamp(this->Pitch - mouseDelta.y * mouseSensitivity, -80.0f, 80.0f);
            float pitchStep = glm::radians(desiredPitch - this->Pitch);
            this->Direction = glm::normalize(glm::rotate(this->Direction, pitchStep, right));
            this->Pitch = desiredPitch;
        }
    }

    // Movement
    if (input.IsKeyPressed(GLFW_KEY_W))
        this->Position += this->Direction * moveStep;
    if (input.IsKeyPressed(GLFW_KEY_S))
        this->Position -= this->Direction * moveStep;
    if (input.IsKeyPressed(GLFW_KEY_A))
        this->Position -= right * moveStep;
    if (input.IsKeyPressed(GLFW_KEY_D))
        this->Position += right * moveStep;
    if (input.IsKeyPressed(GLFW_KEY_LEFT_CONTROL))
        this->Position.y += moveStep;
    if (input.IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
        this->Position.y -= moveStep;

    // Rotation - Yaw (left/right)
    if (input.IsKeyPressed(GLFW_KEY_LEFT))
        this->Direction = glm::normalize(glm::rotate(this->Direction, rotateStep, up));
    if (input.IsKeyPressed(GLFW_KEY_RIGHT))
        this->Direction = glm::normalize(glm::rotate(this->Direction, -rotateStep, up));

    // Rotation - Pitch (up/down), clamped to +-80 degrees
    if (input.IsKeyPressed(GLFW_KEY_UP) && this->Pitch < 80.0f) {
        this->Direction = glm::normalize(glm::rotate(this->Direction, rotateStep, right));
        this->Pitch += glm::degrees(rotateStep);
        if (this->Pitch > 80.0f) this->Pitch = 80.0f;
    }

    if (input.IsKeyPressed(GLFW_KEY_DOWN) && this->Pitch > -80.0f) {
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

Ray Camera::ScreenPointToRay(glm::vec2 screenPos, glm::vec2 screenSize) const {
    float ndcX = (2.0f * screenPos.x) / screenSize.x - 1.0f;
    float ndcY = 1.0f - (2.0f * screenPos.y) / screenSize.y; // screen y-down -> NDC y-up

    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(this->Direction, worldUp));
    glm::vec3 camUp = glm::normalize(glm::cross(right, this->Direction));

    float aspect = screenSize.x / screenSize.y;
    float halfHeight = tanf(glm::radians(this->ProjectionAngle) * 0.5f);
    float halfWidth = halfHeight * aspect;

    glm::vec3 dir = glm::normalize(this->Direction + right * (ndcX * halfWidth) + camUp * (ndcY * halfHeight));
    return Ray{ this->Position, dir };
}
