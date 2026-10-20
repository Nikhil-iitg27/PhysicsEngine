#pragma once
#include <cstdint>
#include <vector>
#include <random>
#include <glm/glm.hpp>
#include "../sphere.h"
#include "../cuboid.h"
#include "../cuboid_mesh.h"
#include "../sphere_mesh.h"
#include "SphereSpawner.h"

// Owns the objects that make up one simulation instance: the dynamic spheres
// and the room's static walls, plus the spawn logic that grows the sphere
// list over time. Replaces the free-standing `spheres`/`walls` vectors and
// free-function `SphereSpawner`/`WallSpawner` that used to live directly in
// Application.cpp's main().
//
// Scene deliberately owns *containers and spawning*, not simulation or
// rendering logic: PhysicsWorld still drives physics on scene.spheres /
// scene.walls, and the render loop still iterates them directly to draw --
// this keeps Scene a thin ownership boundary rather than a second
// god-object standing in for the one Application.cpp used to be.
// Spawn tuning, held by value on Scene so the debug UI has one
// place to read/write instead of literals passed fresh at each call site.
struct SpawnParams {
    float spawnInterval = 0.5f;
    size_t maxSpheres = 50;
};

class Scene {
public:
    std::vector<Sphere> spheres;
    std::vector<Cuboid> walls;
    SpawnParams spawnParams;

    // Builds the fixed 6-wall room (floor, ceiling, and 4 side walls) that
    // bounds the simulation. wallMesh must outlive the Scene, following the
    // existing non-owning-pointer convention Cuboid/Sphere already use for
    // their meshes. Clears any previously built walls first, so calling this
    // again rebuilds the room from scratch.
    //
    // The room's footprint size and wall thickness are read directly from
    // wallMesh's own dimensions (getLength()/getHeight()) rather than being
    // passed as separate parameters here -- wallMesh's constructor is already
    // the one place a caller decides "how big is the room," and duplicating
    // that size as a second, independently-defaulted parameter here was a
    // real bug: the two could silently disagree, since
    // nothing tied them together. wallMesh's length and breadth are assumed
    // equal (a square room footprint) -- see the assertion in BuildRoom's
    // implementation.
    void BuildRoom(Cuboid_mesh* wallMesh);

    // Advances the sphere-spawn timer by deltaTime and appends new spheres
    // (per spawnParams) via this Scene's own SphereSpawner instance -- see
    // SphereSpawner.h for why that timer lives on an owned object instead of
    // a function-local static.
    void SpawnSpheres(Sphere_mesh* mesh, float deltaTime, std::mt19937& gen);

    // Places one sphere directly (no timer/cap gating) -- for interactive
    // placement rather than the automatic spawner.
    uint32_t SpawnSphereAt(Sphere_mesh* mesh, glm::vec3 position, glm::vec3 velocity);

    // Places `count` randomized spheres in one call, no timer/cap gating --
    // for the CLI benchmark/capture mode (Application.cpp's --spheres=N),
    // which needs an exact sphere count up front rather than waiting out the
    // timer-driven spawner. Uses the same random position/velocity
    // distribution as the timer-driven path (see SphereSpawner::SpawnMany).
    void SpawnBatch(Sphere_mesh* mesh, size_t count, std::mt19937& gen);

    // Linear scan by RigidBody::id. Returned pointer is only valid until the
    // next spheres mutation (push_back/RemoveById) -- never store it past
    // the current frame; store the id instead.
    Sphere* FindById(uint32_t id);

    // Swap-and-pop removal by id. Returns false if no sphere had that id.
    bool RemoveById(uint32_t id);

    // Clears spheres and resets the spawn timer (not the id counter -- ids
    // stay unique for the process's lifetime). Walls are left as-is.
    void Reset();

private:
    SphereSpawner sphereSpawner;
};
