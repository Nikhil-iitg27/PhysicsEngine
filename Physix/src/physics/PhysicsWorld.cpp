// See PhysicsWorld.h for the sub-stepped pipeline this drives (broad-phase ->
// narrow-phase/resolve -> integrate) and why sphere-sphere resolution stays
// single-threaded while wall-collision+integration is parallelized; this
// file is the implementation.
#include "PhysicsWorld.h"
#include <limits>

PhysicsWorld::PhysicsWorld(int iterations, int numThreads, float collisionCellSize, float wallThickness)
    : iterations(iterations), wallThickness(wallThickness), broadPhase(collisionCellSize), jobSystem(numThreads) {
}

void PhysicsWorld::ResolveSphereCollisions(std::vector<Sphere>& spheres) {
    if (collisionMode == CollisionMode::BruteForce) {
        ResolveSphereCollisionsBruteForce(spheres);
    } else {
        ResolveSphereCollisionsBroadPhase(spheres);
    }
}

void PhysicsWorld::ResolveSphereCollisionsBroadPhase(std::vector<Sphere>& spheres) {
    // RigidBody::ProcessSphereCollision takes pointers rather than a
    // std::vector<RigidBody> because the bodies here live inside Sphere
    // entities, not in a standalone contiguous array -- see RigidBody.h.
    std::vector<RigidBody*> bodies;
    bodies.reserve(spheres.size());
    for (Sphere& s : spheres) {
        bodies.push_back(&s.body);
    }

    // Rebuilt every sub-step since bodies moved during the previous one.
    broadPhase.Build(bodies);

    // Reused across the loop so each QueryNearby call doesn't reallocate.
    std::vector<RigidBody*> candidates;
    for (Sphere& s : spheres) {
        broadPhase.QueryNearby(&s.body, candidates);
        s.body.ProcessSphereCollision(candidates, restitution);
    }
}

void PhysicsWorld::ResolveSphereCollisionsBruteForce(std::vector<Sphere>& spheres) {
    // Naive O(n^2) candidate search: every body is a candidate for every
    // other body, skipping the broad-phase grid entirely. Exists purely so
    // it can be benchmarked against ResolveSphereCollisionsBroadPhase (see
    // tools/run_ablation.ps1) -- same narrow-phase math, just no pruning.
    std::vector<RigidBody*> bodies;
    bodies.reserve(spheres.size());
    for (Sphere& s : spheres) {
        bodies.push_back(&s.body);
    }

    for (Sphere& s : spheres) {
        s.body.ProcessSphereCollision(bodies, restitution);
    }
}

void PhysicsWorld::IntegrateSphereBatch(std::vector<Sphere>& spheres, const std::vector<Cuboid>& walls,
    float subDeltaTime, float maxSpeed, int threadId, int numThreads) {
    size_t siz = spheres.size();
    size_t batchSize = siz / numThreads;
    size_t left = batchSize * threadId;
    size_t right = (threadId == numThreads - 1) ? siz : batchSize * (threadId + 1);

    for (size_t j = left; j < right; j++) {
        RigidBody& body = spheres[j].body;
        body.ProcessCuboidCollision(walls, restitution);
        body.Update(subDeltaTime, maxSpeed);
        glm::vec3 accel = (accelerationMode == AccelerationMode::UniformGravity)
            ? gravity
            : -springStrength * body.position;
        body.SetAcceleration(accel);
    }
}

void PhysicsWorld::Step(std::vector<Sphere>& spheres, const std::vector<Cuboid>& walls, float deltaTime) {
    if (paused) return;
    StepInternal(spheres, walls, deltaTime);
}

void PhysicsWorld::StepOnce(std::vector<Sphere>& spheres, const std::vector<Cuboid>& walls, float deltaTime) {
    StepInternal(spheres, walls, deltaTime);
}

void PhysicsWorld::StepInternal(std::vector<Sphere>& spheres, const std::vector<Cuboid>& walls, float deltaTime) {
    float subDeltaTime = deltaTime / iterations;
    int numThreads = jobSystem.getNumWorkers();

    // Recomputed from the actual sub-step duration every call (not a fixed
    // constant) so the tunneling guard stays correct even when deltaTime
    // itself is unusually large (a slow/heavily-loaded frame) -- see
    // RigidBody::Update and SetTunnelSafetyFactor's comment.
    float maxSpeed = (subDeltaTime > 0.0f)
        ? tunnelSafetyFactor * wallThickness / subDeltaTime
        : std::numeric_limits<float>::max();

    for (int i = 0; i < iterations; i++) {
        ResolveSphereCollisions(spheres);

        jobSystem.RunParallelFor([this, &spheres, &walls, subDeltaTime, maxSpeed, numThreads](int threadId) {
            IntegrateSphereBatch(spheres, walls, subDeltaTime, maxSpeed, threadId, numThreads);
        });
    }
}
