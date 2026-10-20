// See BroadPhase.h for the uniform-grid spatial-hashing algorithm and its
// cellSize correctness requirement; this file is the implementation.
#include "BroadPhase.h"
#include <cmath>

UniformGrid::UniformGrid(float cellSize) : cellSize(cellSize) {
}

glm::ivec3 UniformGrid::CellCoords(const glm::vec3& position) const {
    return glm::ivec3(
        static_cast<int>(std::floor(position.x / cellSize)),
        static_cast<int>(std::floor(position.y / cellSize)),
        static_cast<int>(std::floor(position.z / cellSize)));
}

UniformGrid::CellKey UniformGrid::PackKey(const glm::ivec3& coords) {
    // Bias each axis into a non-negative range and pack 21 bits per axis into
    // a 63-bit key -- supports cell coordinates in roughly
    // [-1,048,576, 1,048,575] per axis, far beyond what this simulation's
    // bounded room ever needs, with no risk of distinct cells colliding.
    const int64_t bias = 1 << 20;
    int64_t x = static_cast<int64_t>(coords.x) + bias;
    int64_t y = static_cast<int64_t>(coords.y) + bias;
    int64_t z = static_cast<int64_t>(coords.z) + bias;
    return (x << 42) | (y << 21) | z;
}

void UniformGrid::Build(const std::vector<RigidBody*>& bodies) {
    cells.clear();
    for (RigidBody* body : bodies) {
        cells[PackKey(CellCoords(body->position))].push_back(body);
    }
}

void UniformGrid::QueryNearby(const RigidBody* body, std::vector<RigidBody*>& outCandidates) const {
    outCandidates.clear();
    glm::ivec3 center = CellCoords(body->position);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                auto it = cells.find(PackKey(center + glm::ivec3(dx, dy, dz)));
                if (it != cells.end()) {
                    outCandidates.insert(outCandidates.end(), it->second.begin(), it->second.end());
                }
            }
        }
    }
}
