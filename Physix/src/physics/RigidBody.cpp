// See RigidBody.h for what this class is; this file is the implementation of
// its motion and collision algorithms specifically:
//  - Update(): semi-implicit (symplectic) Euler integration -- velocity is
//    updated from acceleration *before* using that new velocity to update
//    position, which is unconditionally stable for this kind of constant/
//    slowly-varying acceleration and cheaper than RK4 or similar.
//  - ProcessCuboidCollision(): signed-distance-to-plane test per wall face
//    (see cuboid.h) with positional correction (push the sphere out along
//    the face normal by the penetration depth) and a perfectly elastic
//    (restitution = 1.0) velocity reflection off that normal.
//  - ProcessSphereCollision(): narrow-phase circle/sphere overlap test
//    (distance between centers < sum of radii) with an impulse-based
//    resolution -- split positional correction (push both spheres apart
//    proportional to penetration) plus a 1D impulse along the collision
//    normal derived from conservation of momentum for a perfectly elastic
//    collision. Candidates are pre-filtered by the broad-phase grid (see
//    collision/BroadPhase.h) before reaching this function.
#include "RigidBody.h"
#include <cmath>

RigidBody::RigidBody(float mass, float radius, glm::vec3 position)
    : mass(mass), radius(radius), position(position), velocity(0.0f), acceleration(0.0f) {
}

void RigidBody::SetVelocity(glm::vec3 velocity) {
    this->velocity = velocity;
}

void RigidBody::SetAcceleration(glm::vec3 acceleration) {
    this->acceleration = acceleration;
}

void RigidBody::SetForce(glm::vec3 force) {
    this->SetAcceleration(acceleration + (force * (1 / mass)));
}

void RigidBody::ToggleFixed() {
    this->fixed = !this->fixed;
}

void RigidBody::Update(float deltaTime, float maxSpeed) {
    if (this->fixed) return;
    // Update velocity with acceleration
    velocity += acceleration * deltaTime;

    // Tunneling guard: clamp *before* using velocity to move position, not
    // after -- clamping afterward would already have let an overshooting
    // position update through. A speed above maxSpeed would move this body
    // farther than a wall's thickness in this one step, which could carry it
    // past ProcessCuboidCollision's plane test without ever being detected
    // as penetrating (that test only looks at the current position, not the
    // swept path between positions).
    float speed = glm::length(velocity);
    if (speed > maxSpeed && speed > 0.0f) {
        velocity *= (maxSpeed / speed);
    }

    // Update position with velocity
    position += velocity * deltaTime;
}

void RigidBody::ProcessCuboidCollision(const std::vector<Cuboid>& cuboids, float restitution) {
    for (const Cuboid& cuboid : cuboids) {
        const glm::mat4& invModel = cuboid.getInverseModelMatrix();

        float hx = cuboid.getLength() * 0.5f;
        float hy = cuboid.getHeight() * 0.5f;
        float hz = cuboid.getBreadth() * 0.5f;

        glm::vec4 localCenter4 = invModel * glm::vec4(position, 1.0f);
        glm::vec3 localCenter = glm::vec3(localCenter4) / localCenter4.w;

        const std::vector<Face>& faces = cuboid.getSurfacePlanes();

        auto inBounds = [&](int i) {
            switch (i) {
            case 0: return (localCenter.x >= -hx && localCenter.x <= hx &&
                localCenter.y >= -hy && localCenter.y <= hy);
            case 1: return (localCenter.x >= -hx && localCenter.x <= hx &&
                localCenter.y >= -hy && localCenter.y <= hy);
            case 2: return (localCenter.z >= -hz && localCenter.z <= hz &&
                localCenter.y >= -hy && localCenter.y <= hy);
            case 3: return (localCenter.z >= -hz && localCenter.z <= hz &&
                localCenter.y >= -hy && localCenter.y <= hy);
            case 4: return (localCenter.x >= -hx && localCenter.x <= hx &&
                localCenter.z >= -hz && localCenter.z <= hz);
            case 5: return (localCenter.x >= -hx && localCenter.x <= hx &&
                localCenter.z >= -hz && localCenter.z <= hz);
            default: return false;
            }
            };

        for (size_t i = 0; i < faces.size(); i++) {
            if (!inBounds(static_cast<int>(i))) continue;
            const Face& face = faces[i];
            float distance = glm::dot(position - face.point, face.normal);
            float r = radius;
            if (std::abs(distance) < r) {
                float pen = r - std::abs(distance) + 0.5f;
                position += (distance < 0 ? -pen : pen) * face.normal;
                velocity -= (1.0f + restitution) * glm::dot(velocity, face.normal) * face.normal;
            }
        }
    }
}

void RigidBody::ProcessSphereCollision(const std::vector<RigidBody*>& bodies, float restitution) {
    for (RigidBody* other : bodies) {
        if (other == this) continue;

        glm::vec3 diff = position - other->position;
        float minDist = radius + other->radius;

        // Squared-distance reject before paying for a sqrt: most candidate
        // pairs the broad-phase grid returns are nearby but not actually
        // overlapping, so this early-out skips glm::length's sqrt for the
        // common non-colliding case (stacks with the uniform-grid
        // broad-phase in physics/collision/BroadPhase.h).
        float distSq = glm::dot(diff, diff);
        if (distSq >= minDist * minDist || distSq <= 0.0f) continue;

        float dist = std::sqrt(distSq);
        glm::vec3 normal = diff / dist; // equivalent to glm::normalize(diff), reusing the dist already computed above
        float penetration = minDist - dist;

        // Resolve penetration (split push)
        position += 0.5f * penetration * normal;
        other->position -= 0.5f * penetration * normal;

        // Relative velocity
        glm::vec3 relativeVel = velocity - other->velocity;
        float velAlongNormal = glm::dot(relativeVel, normal);

        if (velAlongNormal > 0.0f) continue; // Already separating

        // Compute impulse scalar
        float invMass1 = 1.0f / mass;
        float invMass2 = 1.0f / other->mass;

        float j = -(1 + restitution) * velAlongNormal;
        j /= invMass1 + invMass2;

        glm::vec3 impulse = j * normal;

        // Apply impulses
        velocity += impulse * invMass1;
        other->velocity -= impulse * invMass2;
    }
}

glm::vec3 RigidBody::ComputeMomentum() {
    return mass * velocity;
}
