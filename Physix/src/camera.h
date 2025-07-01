#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
private:
    glm::vec3 Position;
    glm::vec3 Direction; // Always normalized
    float Speed = 10.0f;
    float AngularSpeed = 45.0f; // Degrees per second

    float MinDist = 0.1f;
    float MaxDist = 500.0f;

    float ProjectionAngle = 45.0f;
    float Pitch = 0.0f; // In degrees
    float FocusLength = 10.0f;

    int Mode = 0;

public:
    Camera(const glm::vec3& cameraPos = glm::vec3(0.0f, 0.0f, 100.0f),
        const glm::vec3& cameraDir = glm::vec3(0.0f, 0.0f, -1.0f),
        const int mode = 0);

    void detectKeyPress(GLFWwindow* window, float deltaTime);

    float distanceFromCameraPlane(const glm::vec3& point) const;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Getters
    glm::vec3 getPosition() const { return this->Position; }
    glm::vec3 getDirection() const { return this->Direction; }
    float getSpeed() const { return this->Speed; }
    float getAngularSpeed() const { return this->AngularSpeed; }
    float getMinDist() const { return this->MinDist; }
    float getMaxDist() const { return this->MaxDist; }
    float getProjectionAngle() const { return this->ProjectionAngle; }
    float getFocusLength() const { return this->FocusLength; }
    int getMode() const { return this->Mode; }

    // Setters
    void setPosition(const glm::vec3& pos) { this->Position = pos; }
    void setDirection(const glm::vec3& dir) { this->Direction = glm::normalize(dir); }
    void setSpeed(float speed) { this->Speed = speed; }
    void setAngularSpeed(float angularSpeed) { this->AngularSpeed = angularSpeed; }
    void setMinDist(float dist) { this->MinDist = dist; }
    void setMaxDist(float dist) { this->MaxDist = dist; }
    void setProjectionAngle(float angle) { this->ProjectionAngle = angle; }
    void setFocusLength(float length) { this->FocusLength = length; }
    void setMode(int mode) { this->Mode = mode; }
};
