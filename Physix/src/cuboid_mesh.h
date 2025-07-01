#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

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
