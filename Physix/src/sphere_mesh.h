// Sphere.h
#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

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
