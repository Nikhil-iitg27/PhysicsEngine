#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "../sphere.h"
#include "../cuboid.h"
#include "RigidBody.h"
#include "collision/BroadPhase.h"
#include "JobSystem.h"

// Which acceleration model IntegrateSphereBatch applies each sub-step.
// OrbitSpring is this project's original, distinctive default (an inverse
// spring pulling every sphere toward the origin, not real gravity -- kept as
// the default so existing behavior/screenshots don't change). UniformGravity
// is a constant downward (or any-direction) acceleration, the conventional
// alternative.
enum class AccelerationMode { OrbitSpring, UniformGravity };

// Which sphere-sphere collision candidate search PhysicsWorld uses.
// BroadPhase (default) is the tuned UniformGrid spatial hash. BruteForce is a
// naive O(n^2) all-pairs scan, kept as a togglable mode purely so the two can
// be benchmarked against each other (see tools/run_ablation.ps1) -- it reuses
// RigidBody::ProcessSphereCollision unchanged, just widening the candidate
// list to every body instead of the grid's 3x3x3-cell neighborhood.
enum class CollisionMode { BroadPhase, BruteForce };

// Owns the sub-stepped collision-resolution + integration pipeline for the
// dynamic spheres in the scene. Extracted out of Application.cpp's free-function
// ProcessCollisions so main() calls Step() once per frame instead of doubling as
// the physics scheduler.
class PhysicsWorld {
public:
    // collisionCellSize must be >= the largest sphere diameter (2 * radius)
    // that will ever be added to `spheres` -- see UniformGrid. Defaults to
    // 2.0f to match the radius-1.0 spheres Application.cpp's SphereSpawner
    // spawns today; pass an explicit value if that ever changes.
    //
    // wallThickness feeds the tunneling guard (see SetTunnelSafetyFactor) --
    // defaults to 1.0f to match the room Application.cpp builds today
    // (Cuboid_mesh(60, 60, 1.0f)'s thickness parameter); pass the actual
    // value if the room's wall thickness ever changes, same convention as
    // collisionCellSize above.
    PhysicsWorld(int iterations = 5, int numThreads = 10, float collisionCellSize = 2.0f, float wallThickness = 1.0f);

    // Advances every sphere by deltaTime, split into `iterations` sub-steps.
    // No-op while paused -- see StepOnce for frame-by-frame advancement.
    void Step(std::vector<Sphere>& spheres, const std::vector<Cuboid>& walls, float deltaTime);

    // Runs one Step regardless of the paused flag.
    void StepOnce(std::vector<Sphere>& spheres, const std::vector<Cuboid>& walls, float deltaTime);

    void SetPaused(bool value) { paused = value; }
    bool IsPaused() const { return paused; }

    // 1.0 = perfectly elastic (default), 0.0 = perfectly inelastic.
    void SetRestitution(float value) { restitution = value; }
    float GetRestitution() const { return restitution; }

    void SetAccelerationMode(AccelerationMode mode) { accelerationMode = mode; }
    AccelerationMode GetAccelerationMode() const { return accelerationMode; }

    void SetCollisionMode(CollisionMode mode) { collisionMode = mode; }
    CollisionMode GetCollisionMode() const { return collisionMode; }
    // Only used in UniformGravity mode.
    void SetGravity(glm::vec3 value) { gravity = value; }
    glm::vec3 GetGravity() const { return gravity; }
    // Only used in OrbitSpring mode; matches the original hardcoded -10.0f.
    void SetSpringStrength(float value) { springStrength = value; }
    float GetSpringStrength() const { return springStrength; }

    // Tunneling guard: each sub-step, every sphere's velocity is capped so it
    // can move at most (tunnelSafetyFactor * wallThickness) in that
    // sub-step's duration -- a fast-enough sphere could otherwise cross a
    // wall's entire thickness between one position sample and the next,
    // skipping past RigidBody::ProcessCuboidCollision's plane test
    // undetected (that test only checks the current position, not the swept
    // path). Recomputed live from the *actual* sub-step duration every step
    // (not a fixed constant derived once), so it stays correct even during a
    // frame-rate drop or a heavily-loaded run (e.g. the brute-force/
    // high-sphere-count ablation sweep) rather than only at whatever frame
    // rate a constant happened to be tuned for. 0.9 default leaves a 10%
    // margin below the exact tunneling threshold. See RigidBody::Update.
    void SetTunnelSafetyFactor(float value) { tunnelSafetyFactor = value; }
    float GetTunnelSafetyFactor() const { return tunnelSafetyFactor; }
    void SetWallThickness(float value) { wallThickness = value; }
    float GetWallThickness() const { return wallThickness; }

    int getIterations() const { return iterations; }
    // Source of truth is the job system's actual worker count, not a
    // separately-tracked number -- see the removed setNumThreads note below.
    int getNumThreads() const { return jobSystem.getNumWorkers(); }
    void setIterations(int iterations) { this->iterations = iterations; }

private:
    int iterations;
    bool paused = false;
    float restitution = 1.0f;
    AccelerationMode accelerationMode = AccelerationMode::OrbitSpring;
    CollisionMode collisionMode = CollisionMode::BroadPhase;
    glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);
    float springStrength = 10.0f;
    float wallThickness;
    float tunnelSafetyFactor = 0.9f;
    UniformGrid broadPhase;
    // Persistent worker pool sized once at construction. There is no
    // setNumThreads (removed -- it had zero call sites in the codebase):
    // JobSystem's pool size is fixed once its worker threads are created, so
    // a setter that only updated a separate `numThreads` field would silently
    // desync IntegrateSphereBatch's partitioning math from the pool's actual
    // worker count. Construct a new PhysicsWorld instead if a different
    // thread count is ever needed.
    JobSystem jobSystem;

    // Sphere-sphere collision mutates *both* spheres in a pair (position/velocity
    // of `other`, not just the caller), so it cannot be split across threads by
    // index range without further work partitioning the broad-phase itself by
    // cell (see UniformGrid's thread-safety note). Runs single-threaded for now.
    // Dispatches to one of the two candidate-search strategies below based on
    // `collisionMode`; both call the same RigidBody::ProcessSphereCollision.
    void ResolveSphereCollisions(std::vector<Sphere>& spheres);
    void ResolveSphereCollisionsBroadPhase(std::vector<Sphere>& spheres);
    void ResolveSphereCollisionsBruteForce(std::vector<Sphere>& spheres);

    // Wall collision + integration only ever read shared state (the walls) and
    // write to the sphere at index j itself, so this part is safe to split
    // across threads by index range.
    void IntegrateSphereBatch(std::vector<Sphere>& spheres, const std::vector<Cuboid>& walls,
        float subDeltaTime, float maxSpeed, int threadId, int numThreads);

    // Shared body for Step/StepOnce -- Step checks `paused` before calling this.
    void StepInternal(std::vector<Sphere>& spheres, const std::vector<Cuboid>& walls, float deltaTime);
};
