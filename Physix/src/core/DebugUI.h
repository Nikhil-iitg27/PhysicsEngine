#pragma once
struct GLFWwindow;

// Owns Dear ImGui's context lifecycle and the per-frame BeginFrame/EndFrame
// bracket. Kept separate from Window (which owns the GLFW window/GL context)
// -- this is a second, unrelated lifecycle (a UI framework) that happens to
// need the same handles, not a reason to fold it into Window.
// UI content itself (panels/sliders) is drawn between BeginFrame/EndFrame by
// the caller (Application.cpp), not by this class.
class DebugUI {
public:
    explicit DebugUI(GLFWwindow* window);
    ~DebugUI();

    DebugUI(const DebugUI&) = delete;
    DebugUI& operator=(const DebugUI&) = delete;

    void BeginFrame();
    void EndFrame();
};
