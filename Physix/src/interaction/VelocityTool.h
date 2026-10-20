#pragma once
#include <glm/glm.hpp>

// Drag gesture state for imbuing the selected sphere with velocity: press on
// the already-selected sphere begins a drag, release computes a velocity
// from the screen-space drag and applies it (see Application.cpp for the
// press/release wiring and the drag-to-velocity math, which needs Camera).
// Grew past a bare local into its own class once it needed more than one
// piece of cross-frame state (dragging + drag start).
class VelocityTool {
public:
    void BeginDrag(glm::vec2 screenPos) { dragging = true; dragStart = screenPos; }
    void CancelDrag() { dragging = false; }
    bool IsDragging() const { return dragging; }
    glm::vec2 GetDragStart() const { return dragStart; }

private:
    bool dragging = false;
    glm::vec2 dragStart = glm::vec2(0.0f);
};
