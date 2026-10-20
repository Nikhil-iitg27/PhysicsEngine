#pragma once
#include <cstdint>

// Tracks which sphere (by RigidBody::id) is currently selected, if any (0 =
// none). Not Scene state (data-ownership vs. UI/interaction-state), not a
// bare local (velocity-imbuing needs cross-frame state alongside
// this). Header-only: currently trivial, same
// precedent as Input before it grew real logic.
class Selection {
public:
    void Set(uint32_t id) { selectedId = id; }
    void Clear() { selectedId = 0; }
    uint32_t Get() const { return selectedId; }
    bool HasSelection() const { return selectedId != 0; }

private:
    uint32_t selectedId = 0;
};
