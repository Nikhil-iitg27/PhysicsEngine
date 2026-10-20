#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
#include <limits>
#include "../cuboid.h"

// Pure physical state and motion for a spherical dynamic body: mass, radius,
// position, velocity, acceleration, and the collision detection/resolution
// that mutates them. Deliberately has no knowledge of meshes, shaders, or
// rendering -- see render/RenderComponent.h for that half, and sphere.h for
// how the two are paired per-entity. Keeping RigidBody free of rendering
// types is what lets PhysicsWorld (and future physics tests) operate
// without a GL context.
class RigidBody {
public:
    // 0 = no id (sentinel). Assigned by SphereSpawner at spawn time; stable
    // for the body's lifetime, used by Scene::FindById/RemoveById and
    // interaction/Selection.
    uint32_t id = 0;
    float mass;
    float radius;
    bool fixed = false;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;

    RigidBody(float mass, float radius, glm::vec3 position);

    void SetVelocity(glm::vec3 velocity);
    void SetAcceleration(glm::vec3 acceleration);
    void SetForce(glm::vec3 force);
    void ToggleFixed();

    // maxSpeed clamps velocity magnitude before it's used to move position
    // this call -- a tunneling guard (PhysicsWorld computes it per sub-step
    // from wall thickness / sub-step duration, see PhysicsWorld.cpp) so a
    // sphere can never travel farther than a wall's thickness in one
    // integration step, which is what would let it skip past a wall's plane
    // test undetected. Defaults to "no cap" so existing/test call sites are
    // unaffected.
    void Update(float deltaTime, float maxSpeed = std::numeric_limits<float>::max());

    // restitution: 1.0 = perfectly elastic, 0.0 = perfectly inelastic.
    // Passed in rather than hardcoded so PhysicsWorld can tune it --
    // previously each of these two methods hardcoded its own
    // separate 1.0f local, with nothing keeping them equal to each other.
    void ProcessCuboidCollision(const std::vector<Cuboid>& cuboids, float restitution = 1.0f);

    // Takes pointers rather than a std::vector<RigidBody> so callers (e.g.
    // PhysicsWorld) can build the candidate list from bodies that live inside
    // other owning objects (like Sphere) without RigidBody needing to know
    // about those owners.
    void ProcessSphereCollision(const std::vector<RigidBody*>& bodies, float restitution = 1.0f);

private:
    glm::vec3 ComputeMomentum();
};
