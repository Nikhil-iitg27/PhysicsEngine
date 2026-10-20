#pragma once
#include <cstdint>
#include <vector>
#include <random>
#include "../sphere.h"
#include "../sphere_mesh.h"

// Spawns spheres into a sphere list, assigning each a stable id (see
// RigidBody::id). Update() is the timer-gated, randomized spawn path;
// SpawnOne() is the primitive it's built on, also used directly for
// user-driven placement.
class SphereSpawner {
public:
    // Random position/velocity, timer-gated (spawnInterval), capped at maxSpheres.
    void Update(std::vector<Sphere>& spheres, Sphere_mesh* mesh, float deltaTime,
        float spawnInterval, size_t maxSpheres, std::mt19937& gen);

    // Appends one sphere with an explicit position/velocity; no timer/cap
    // gating. Returns the new sphere's id.
    uint32_t SpawnOne(std::vector<Sphere>& spheres, Sphere_mesh* mesh,
        glm::vec3 position, glm::vec3 velocity,
        glm::vec3 color = glm::vec3(0.0f, 0.0f, 1.0f), float transparency = 1.0f);

    // Appends `count` spheres with the same randomized position/velocity
    // distribution Update() uses, in one call, no timer/cap gating -- see
    // Scene::SpawnBatch.
    void SpawnMany(std::vector<Sphere>& spheres, Sphere_mesh* mesh, size_t count, std::mt19937& gen);

    // Resets spawnTimer only -- nextId is untouched (see its comment below).
    void ResetTimer() { spawnTimer = 0.0f; }

private:
    void SpawnRandom(std::vector<Sphere>& spheres, Sphere_mesh* mesh, std::mt19937& gen);

    float spawnTimer = 0.0f;
    // Starts at 1; 0 is reserved as RigidBody::id's "no id" sentinel. Never
    // reset (not even by Scene::Reset) so ids stay unique for the process's
    // whole lifetime.
    uint32_t nextId = 1;
};
