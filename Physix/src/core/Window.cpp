// See Window.h for what this class owns; this file is the implementation,
// including the GLFW-callback-to-instance trampoline (FramebufferSizeCallback).
#include "Window.h"
#include <iostream>

Window::Window(unsigned int width, unsigned int height, const char* title, bool visible)
    : width(width), height(height) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return;
    }

    // Configure GLFW for OpenGL 3.3 Core Profile.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

    handle = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title, nullptr, nullptr);
    if (!handle) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(handle);

    glfwSetWindowUserPointer(handle, this);
    glfwSetFramebufferSizeCallback(handle, &Window::FramebufferSizeCallback);

    // Load GLAD after creating the OpenGL context.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        handle = nullptr; // window/context already exist; caller must still check IsValid() and exit
        return;
    }

    // The actual framebuffer can be larger than the width/height passed to
    // glfwCreateWindow whenever the display's content scale isn't 100% (the
    // same DPI/screen-vs-pixel distinction as GetContentWidth/Height's note
    // below) -- glfwCreateWindow's arguments are in screen coordinates, not
    // framebuffer pixels. FramebufferSizeCallback only fires on a later
    // resize, not at creation, so without this query `width`/`height` (and
    // the glViewport call right after) would silently stay at the smaller
    // screen-coordinate values for the entire run: glm::perspective's aspect
    // ratio would be wrong and glViewport would leave part of the real
    // framebuffer unrendered (visible as black borders in any
    // glReadPixels-based capture -- see core/Capture.h).
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(handle, &fbWidth, &fbHeight);
    if (fbWidth > 0 && fbHeight > 0) {
        this->width = static_cast<unsigned int>(fbWidth);
        this->height = static_cast<unsigned int>(fbHeight);
    }

    glViewport(0, 0, static_cast<int>(this->width), static_cast<int>(this->height));
}

Window::~Window() {
    if (handle) {
        glfwDestroyWindow(handle);
    }
    // Safe even if glfwInit() never succeeded or was already torn down --
    // per GLFW's documented contract, glfwTerminate() may be called before
    // glfwInit() and repeated calls after a successful termination are a
    // no-op.
    glfwTerminate();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(handle);
}

void Window::SwapBuffers() {
    glfwSwapBuffers(handle);
}

void Window::PollEvents() {
    glfwPollEvents();
}

float Window::GetAspectRatio() const {
    // Defense in depth: height should never be 0 (FramebufferSizeCallback
    // below already refuses to store a 0 size), but guard here too so a
    // zero/degenerate aspect can never reach glm::perspective(), which
    // asserts on it (confirmed by an actual crash: minimizing the window
    // fires GLFW's framebuffer-size callback with 0x0, and an unguarded
    // 0-aspect projection matrix is undefined/asserts).
    if (height == 0) return 1.0f;
    return static_cast<float>(width) / static_cast<float>(height);
}

unsigned int Window::GetContentWidth() const {
    int w = 0, h = 0;
    glfwGetWindowSize(handle, &w, &h);
    return static_cast<unsigned int>(w);
}

unsigned int Window::GetContentHeight() const {
    int w = 0, h = 0;
    glfwGetWindowSize(handle, &w, &h);
    return static_cast<unsigned int>(h);
}

void Window::FramebufferSizeCallback(GLFWwindow* glfwWindow, int newWidth, int newHeight) {
    // Minimizing the window fires this callback with 0x0. Ignore degenerate
    // sizes entirely -- keep the last real size instead of storing 0, so
    // GetAspectRatio() (and glm::perspective downstream) never sees a
    // zero/undefined aspect ratio. Restoring from minimize fires this again
    // with the real size, which is handled normally.
    if (newWidth <= 0 || newHeight <= 0) return;

    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (self) {
        self->width = static_cast<unsigned int>(newWidth);
        self->height = static_cast<unsigned int>(newHeight);
    }
    glViewport(0, 0, newWidth, newHeight);
}
