#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Cuboid_mesh: pure GPU geometry for a box -- generates 24 vertices (4 per
// face x 6 faces, each face given its own copy of vertices so it can have a
// flat per-face normal for correct lighting, rather than sharing/averaging
// vertices at the 8 corners) and 36 indices (2 triangles per face), then
// uploads them to a VAO/VBO/EBO once at construction. No simulation state --
// Cuboid (cuboid.h) holds a non-owning pointer to a mesh like this one and
// supplies the position/rotation; many Cuboid instances can share one mesh
// (this project's room walls all reference the same Cuboid_mesh).
class Cuboid_mesh {
public:
    // Constructor: creates a cuboid mesh with the given dimensions.
    Cuboid_mesh(float length, float breadth, float height);

    // Destructor: cleans up allocated OpenGL resources.
    ~Cuboid_mesh();

    // Render the cuboid mesh.
    void render() const;

    // Getters for dimensions.
    float getLength() const;
    float getBreadth() const;
    float getHeight() const;

private:
    unsigned int VAO, VBO, EBO;
    unsigned int indexCount;
    float length, breadth, height;

    // Helper function to generate vertices (with normals) and indices for the cuboid.
    void generateCuboidMesh(float length, float breadth, float height,
        std::vector<float>& vertices,
        std::vector<unsigned int>& indices);
};
