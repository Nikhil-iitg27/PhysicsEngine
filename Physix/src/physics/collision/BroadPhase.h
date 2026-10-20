#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "../RigidBody.h"

// Uniform spatial grid used as the broad phase for sphere-sphere collision:
// buckets bodies into fixed-size 3D cells so QueryNearby only has to return
// bodies from the 3x3x3 neighborhood of cells around a query point, instead
// of every body in the scene (replaces the O(n^2) all-pairs scan).
//
// Correctness requirement: `cellSize` passed to the constructor must be >=
// the largest sphere diameter (2 * radius) present in the scene. Given that,
// any two overlapping spheres are guaranteed to have centers in the same
// cell or a directly (face/edge/corner) adjacent one, so the 3x3x3
// neighborhood search in QueryNearby cannot miss a real collision. This grid
// does not inspect body radii itself or auto-size cells -- the caller
// (PhysicsWorld) is responsible for choosing a cellSize consistent with the
// bodies it will hold.
//
// Not thread-safe: Build/QueryNearby assume single-threaded use. This is
// fine today because PhysicsWorld only ever drives the broad phase from its
// single-threaded ResolveSphereCollisions pass (see PhysicsWorld.h on why
// sphere-sphere resolution isn't parallelized); a future
// parallel broad-phase would partition work by cell rather than share one
// UniformGrid across threads.
class UniformGrid {
public:
    explicit UniformGrid(float cellSize);

    // Clears and rebuilds the grid from the given bodies' current positions.
    // Call once per broad-phase pass, since bodies move between calls.
    void Build(const std::vector<RigidBody*>& bodies);

    // Fills outCandidates (cleared first) with every body sharing `body`'s
    // cell or a directly adjacent cell -- including `body` itself, since
    // narrow-phase callers (RigidBody::ProcessSphereCollision) already skip
    // self-checks.
    void QueryNearby(const RigidBody* body, std::vector<RigidBody*>& outCandidates) const;

private:
    using CellKey = int64_t;

    float cellSize;
    std::unordered_map<CellKey, std::vector<RigidBody*>> cells;

    glm::ivec3 CellCoords(const glm::vec3& position) const;
    static CellKey PackKey(const glm::ivec3& coords);
};
