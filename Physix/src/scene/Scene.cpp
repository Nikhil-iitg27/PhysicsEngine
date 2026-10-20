// See Scene.h for what this class owns; this file is the implementation,
// including the fixed 6-wall room layout in BuildRoom below.
#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>

void Scene::BuildRoom(Cuboid_mesh* wallMesh) {
    walls.clear();

    // Single source of truth for room size: read it back from the mesh
    // itself instead of taking a separately-defaulted parameter that could
    // silently disagree with it.
    assert(wallMesh->getLength() == wallMesh->getBreadth() &&
        "Scene::BuildRoom assumes a square room footprint (wallMesh length == breadth)");
    float regionSize = wallMesh->getLength();
    float thickness = wallMesh->getHeight();

    float halfSize = regionSize / 2.0f;

    // Floor (XZ plane, no rotation)
    walls.emplace_back(wallMesh,
        glm::vec3(0.0f, -halfSize - thickness / 2.0f, 0.0f),
        glm::vec3(0.0f));

    // Ceiling (XZ plane, no rotation)
    walls.emplace_back(wallMesh,
        glm::vec3(0.0f, halfSize + thickness / 2.0f, 0.0f),
        glm::vec3(0.0f));

    // Left wall (YZ plane), rotated +90 degrees around Y
    walls.emplace_back(wallMesh,
        glm::vec3(-halfSize - thickness / 2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, glm::radians(90.0f)));

    // Right wall (YZ plane), rotated -90 degrees around Y
    walls.emplace_back(wallMesh,
        glm::vec3(halfSize + thickness / 2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, glm::radians(-90.0f)));

    // Back wall (XY plane), rotated -90 degrees around X
    walls.emplace_back(wallMesh,
        glm::vec3(0.0f, 0.0f, -halfSize - thickness / 2.0f),
        glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f));

    // Front wall (XY plane), rotated +90 degrees around X
    walls.emplace_back(wallMesh,
        glm::vec3(0.0f, 0.0f, halfSize + thickness / 2.0f),
        glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
}

void Scene::SpawnSpheres(Sphere_mesh* mesh, float deltaTime, std::mt19937& gen) {
    // Enforce the cap on the shrink side too: spawnParams.maxSpheres used to
    // only gate SphereSpawner::Update's growth, so lowering it below the
    // current count (e.g. via the debug panel) had no effect on spheres
    // already spawned. Trimming here (called every frame already) means the
    // cap is a real invariant, not just a growth limit.
    if (spheres.size() > spawnParams.maxSpheres) {
        // erase(), not resize(): resize() requires Sphere to be default-
        // constructible even for a shrink-only call (a template-
        // instantiation requirement of its "value-initialize new elements"
        // code path, not a runtime one -- confirmed by the compiler
        // rejecting resize() outright, since Sphere has no default
        // constructor by design). erase() only needs move-assignable.
        spheres.erase(spheres.begin() + spawnParams.maxSpheres, spheres.end());
    }
    sphereSpawner.Update(spheres, mesh, deltaTime, spawnParams.spawnInterval, spawnParams.maxSpheres, gen);
}

uint32_t Scene::SpawnSphereAt(Sphere_mesh* mesh, glm::vec3 position, glm::vec3 velocity) {
    return sphereSpawner.SpawnOne(spheres, mesh, position, velocity);
}

void Scene::SpawnBatch(Sphere_mesh* mesh, size_t count, std::mt19937& gen) {
    sphereSpawner.SpawnMany(spheres, mesh, count, gen);
}

Sphere* Scene::FindById(uint32_t id) {
    for (Sphere& s : spheres) {
        if (s.body.id == id) return &s;
    }
    return nullptr;
}

bool Scene::RemoveById(uint32_t id) {
    for (size_t i = 0; i < spheres.size(); i++) {
        if (spheres[i].body.id == id) {
            spheres[i] = spheres.back();
            spheres.pop_back();
            return true;
        }
    }
    return false;
}

void Scene::Reset() {
    spheres.clear();
    sphereSpawner.ResetTimer();
}
