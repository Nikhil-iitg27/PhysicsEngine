#pragma once
#include <glad/glad.h>      // Must be included before GLFW
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>

// Thin wrapper over GLFW's keyboard/mouse state, so callers like Camera
// don't depend on GLFWwindow*/GLFW directly.
class Input {
public:
    explicit Input(GLFWwindow* window) : window(window) {}

    bool IsKeyPressed(int key) const {
        return glfwGetKey(window, key) == GLFW_PRESS;
    }

    bool IsMouseButtonPressed(int button) const {
        return glfwGetMouseButton(window, button) == GLFW_PRESS;
    }

    // True only on the frame a key transitions from up to down -- for
    // toggle/trigger actions (pause, step, reset), not continuous movement.
    // Assumes each key is queried at most once per frame.
    bool WasKeyJustPressed(int key) {
        bool isDown = glfwGetKey(window, key) == GLFW_PRESS;
        bool& wasDown = previousState[key];
        bool justPressed = isDown && !wasDown;
        wasDown = isDown;
        return justPressed;
    }

    // Same as WasKeyJustPressed but for mouse buttons -- a separate map so a
    // key code and a button code (both plain ints) can never collide.
    bool WasMouseButtonJustPressed(int button) {
        bool isDown = glfwGetMouseButton(window, button) == GLFW_PRESS;
        bool& wasDown = previousMouseButtonState[button];
        bool justPressed = isDown && !wasDown;
        wasDown = isDown;
        return justPressed;
    }

    // Current cursor position in screen pixels (origin top-left, y-down).
    // Unlike GetMouseDelta, this has no side effects on delta tracking.
    glm::vec2 GetMousePosition() const {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        return glm::vec2(static_cast<float>(x), static_cast<float>(y));
    }

    // Cursor movement since the last call. Returns zero on the first call
    // after construction or after SetCursorCaptured(true) (avoids a large
    // spurious jump from wherever the cursor happened to be).
    glm::vec2 GetMouseDelta() {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        glm::vec2 pos(static_cast<float>(x), static_cast<float>(y));
        if (!mouseInitialized) {
            lastMousePos = pos;
            mouseInitialized = true;
            return glm::vec2(0.0f);
        }
        glm::vec2 delta = pos - lastMousePos;
        lastMousePos = pos;
        return delta;
    }

    void SetCursorCaptured(bool captured) {
        glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        mouseInitialized = false; // avoid a jump on the next GetMouseDelta
    }

    bool IsCursorCaptured() const {
        return glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    }

private:
    GLFWwindow* window; // non-owning; Window (core/Window.h) owns the handle
    std::unordered_map<int, bool> previousState;
    std::unordered_map<int, bool> previousMouseButtonState;
    glm::vec2 lastMousePos = glm::vec2(0.0f);
    bool mouseInitialized = false;
};
