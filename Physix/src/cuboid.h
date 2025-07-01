#pragma once
#include "cuboid_mesh.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

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

    // Get dimensions from the mesh.
    float getLength() const;
    float getBreadth() const;
    float getHeight() const;

    // Set transformation.
    void setPosition(const glm::vec3& newPosition);
    void setRotation(const glm::vec3& newRotation);

    // Compute and return the world-space planes for each face.
    // Each Face contains a point and the outward normal.
    std::vector<Face> getSurfacePlanes() const;

private:
    Cuboid_mesh* mesh;
    glm::vec3 position;
    glm::vec3 rotation;  // Euler angles in radians.
    glm::mat4 modelMatrix;

    // Update the model matrix based on current position and rotation.
    void updateModelMatrix();
};
