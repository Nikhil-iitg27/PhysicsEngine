#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Sphere_mesh: pure GPU geometry for a sphere -- builds a standard UV-sphere
// (latitude/longitude triangulation: `stacks` horizontal rings from the north
// to south pole, `sectors` vertices per ring, positions parameterized by
// spherical coordinates and normals equal to the normalized position since
// a sphere centered at the origin has an outward normal identical to its
// surface point direction), uploads it to a VAO/VBO/EBO once at construction.
// No simulation state -- Sphere (sphere.h) pairs a RigidBody (physics) with a
// RenderComponent (render/RenderComponent.h) that holds a non-owning pointer
// to a mesh like this one; many spheres can share one mesh (this project
// creates one high-poly and one low-poly Sphere_mesh, shared across every
// spawned sphere).
class Sphere_mesh {
public:
    // Constructor: creates the sphere with given radius, sectors (longitude), and stacks (latitude)
    Sphere_mesh(float radius, int sectors, int stacks);

    // Destructor: cleans up allocated OpenGL resources
    ~Sphere_mesh();

    // Render the sphere
    void render() const;
    float getRadius() const;
    std::pair<unsigned int, unsigned int> getRes() const;

private:
    unsigned int VAO, VBO, EBO;
    unsigned int sectors, stacks;
    unsigned int indexCount;
    float radius;

    // Helper function to generate sphere vertices and indices
    void generateSphereMesh(float radius, int sectors, int stacks,
        std::vector<float>& vertices, std::vector<unsigned int>& indices);
};
