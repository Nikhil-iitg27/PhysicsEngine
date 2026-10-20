#pragma once
#include <glad/glad.h>      // Must be included before GLFW
#include <GLFW/glfw3.h>

// Owns the GLFW window and its OpenGL context, and tracks the framebuffer
// size GLFW reports back on resize. Replaces the free-standing
// SCR_WIDTH/SCR_HEIGHT globals, the framebuffer_size_callback free function,
// and the initialize() free function that used to live directly in
// Application.cpp.
//
// GLFW's resize callback is a plain C function pointer with no way to bind a
// this-pointer directly, so Window registers a static trampoline
// (FramebufferSizeCallback) and uses glfwSetWindowUserPointer to route the
// callback back to the owning instance.
class Window {
public:
    // Initializes GLFW, creates the window and GL context, and loads GLAD.
    // Construction can fail (GLFW init, window creation, or GLAD loading can
    // each fail) -- callers must check IsValid() before using the window
    // further, mirroring the old initialize() function's nullptr-return
    // contract instead of throwing.
    // visible=false creates the window with GLFW_VISIBLE hinted off -- a
    // real GL context still exists and rendering/glReadPixels work
    // identically, nothing is ever shown on screen. Used by the CLI
    // benchmark/capture mode (Application.cpp's --hidden) so dozens of
    // automated runs (see tools/run_ablation.ps1) don't flash windows.
    Window(unsigned int width, unsigned int height, const char* title, bool visible = true);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool IsValid() const { return handle != nullptr; }
    GLFWwindow* GetHandle() const { return handle; }

    bool ShouldClose() const;
    void SwapBuffers();
    void PollEvents();

    // Framebuffer size in pixels -- what glViewport()/aspect-ratio math needs.
    unsigned int GetWidth() const { return width; }
    unsigned int GetHeight() const { return height; }
    float GetAspectRatio() const;

    // Window content-area size in *screen coordinates*, queried live via
    // glfwGetWindowSize -- NOT the same as GetWidth()/GetHeight() whenever
    // the display's DPI/content scale isn't 100% (framebuffer pixels can be
    // a scaled multiple of screen coordinates). glfwGetCursorPos (see
    // Input::GetMousePosition) reports in this same screen-coordinate space,
    // so screen-to-world picking math (Camera::ScreenPointToRay) must use
    // this, not GetWidth()/GetHeight(), or the two mismatch and picking is
    // systematically off -- a real bug found by a user report, not
    // hypothetical.
    unsigned int GetContentWidth() const;
    unsigned int GetContentHeight() const;

private:
    GLFWwindow* handle = nullptr;
    unsigned int width;
    unsigned int height;

    static void FramebufferSizeCallback(GLFWwindow* glfwWindow, int newWidth, int newHeight);
};
