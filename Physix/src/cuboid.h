#pragma once
#include "cuboid_mesh.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// Cuboid: a positioned/rotated box in the scene (used for the room's floor,
// ceiling, and 4 walls; see scene/Scene.h's BuildRoom). Holds a transform
// (position + Euler-angle rotation) and a non-owning pointer to a shared
// Cuboid_mesh for its dimensions/geometry.
//
// Its role in collision detection: for each of its 6 faces, Cuboid exposes a
// world-space Face (a point on the plane + outward normal). Sphere-vs-wall
// collision (RigidBody::ProcessCuboidCollision, in physics/RigidBody.cpp) is
// a signed-distance-to-plane test against these faces -- a sphere is
// penetrating a face if the signed distance from its center to that plane is
// less than its radius. The model matrix, its inverse, and these face planes
// are all cached and only recomputed when the cuboid's transform actually
// changes (see updateModelMatrix), since walls are built once and never move.
//
// A simple structure representing a face (plane) in world space.
// 'point' is a point on the face and 'normal' is the outward-facing normal.
struct Face {
    glm::vec3 point;
    glm::vec3 normal;
};

// The Cuboid class stores physical properties such as position and rotation,
// computes its model matrix, and allows access to the world-space planes of its faces.
class Cuboid {
public:
    // Constructor: accepts a pointer to a Cuboid_mesh (which stores dimensions)
    // and optional position and rotation (Euler angles in radians).
    Cuboid(Cuboid_mesh* mesh,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f));

    // Render the cuboid (delegates to the mesh).
    void Render(const Shader& shader);

    // Returns the model matrix (world transformation) of the cuboid.
    glm::mat4 getModelMatrix() const;

    // Returns the cached inverse of the model matrix. Recomputed only when the
    // cuboid's position/rotation change (see updateModelMatrix), so repeated
    // collision checks against a static cuboid (e.g. a room wall) don't each
    // pay for a 4x4 matrix inverse.
    const glm::mat4& getInverseModelMatrix() const;

    // Get dimensions from the mesh.
    float getLength() const;
    float getBreadth() const;
    float getHeight() const;

    // Set transformation.
    void setPosition(const glm::vec3& newPosition);
    void setRotation(const glm::vec3& newRotation);

    // Returns the cached world-space planes for each face (point + outward normal).
    // Recomputed only when position/rotation change, not on every call.
    const std::vector<Face>& getSurfacePlanes() const;

private:
    Cuboid_mesh* mesh;
    glm::vec3 position;
    glm::vec3 rotation;  // Euler angles in radians.
    glm::mat4 modelMatrix;
    glm::mat4 invModelMatrix;
    std::vector<Face> cachedFaces;

    // Update the model matrix (and the cached inverse + face planes derived from it)
    // based on current position and rotation.
    void updateModelMatrix();
    std::vector<Face> computeSurfacePlanes() const;
};
