#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/Input.h" // includes glad/GLFW in the right order

// World-space ray: origin + normalized direction. Used for picking (see
// Camera::ScreenPointToRay, interaction/Picker.h).
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

// Camera: a free-fly (spectator-style) camera -- position plus a normalized
// look Direction, no target/orbit point. Movement (WASD + Ctrl/Shift for
// up/down) is relative to the current look direction; look direction itself
// is steered by directly rotating the Direction vector around the world-up
// axis (yaw, arrow left/right) or the camera's local right axis (pitch,
// arrow up/down, clamped to +-80 degrees to avoid gimbal-flip at the poles).
// This is simpler than a full quaternion or Euler-angle-matrix camera since
// there's no roll to track. getViewMatrix/getProjectionMatrix build the
// standard glm::lookAt / glm::perspective matrices from that state each
// frame; Update is the only place per-frame input is read.
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

    // Reads WASD + Ctrl/Shift for movement, arrow keys for look, and mouse
    // delta for look while Input::IsCursorCaptured() (mouse-look is gated on
    // capture so the cursor is free to click/select otherwise -- see
    // Application.cpp for what toggles capture).
    void Update(Input& input, float deltaTime);

    float distanceFromCameraPlane(const glm::vec3& point) const;

    // Converts a screen-space point (pixels, origin top-left, y-down --
    // matches GLFW cursor coordinates) into a world-space ray through that
    // point, for picking. screenSize is the window's (width, height).
    Ray ScreenPointToRay(glm::vec2 screenPos, glm::vec2 screenSize) const;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Getters
    glm::vec3 getPosition() const { return this->Position; }
    glm::vec3 getDirection() const { return this->Direction; }
    glm::vec3 getRight() const { return glm::normalize(glm::cross(this->Direction, glm::vec3(0.0f, 1.0f, 0.0f))); }
    glm::vec3 getUp() const { return glm::normalize(glm::cross(getRight(), this->Direction)); }
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
