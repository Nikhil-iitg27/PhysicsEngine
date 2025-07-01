#include "sphere_mesh.h"
#include <cmath>
#include <iostream>

Sphere_mesh::Sphere_mesh(float radius, int sectors, int stacks) : VAO(0), VBO(0), EBO(0), indexCount(0) {
	
	std::vector<float> vertices;
	std::vector<unsigned int> indices;

    this->radius = radius;
    this->sectors = sectors; this->stacks = stacks;

	generateSphereMesh(radius, sectors, stacks, vertices, indices);
	indexCount = static_cast<unsigned int>(indices.size());

	// generate and bind VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// create and populate VBO with vertex data (6 float numbers)
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	// Create and fill EBO with indices
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	// Vertex Attribute Pointers
	// Position attribute (location = 0): 3 floats, stride = 6 floats
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// Normal attribute (location = 1): next 3 floats
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Unbind VAO as EBO remains bound to VAO
	glBindVertexArray(0);
}

Sphere_mesh::~Sphere_mesh() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

void Sphere_mesh::render() const {
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

float Sphere_mesh::getRadius() const{
    return this->radius;
}

std::pair<unsigned int, unsigned int> Sphere_mesh::getRes() const {
    return std::make_pair(this->sectors, this->stacks);
}

void Sphere_mesh::generateSphereMesh(float radius, int sectors, int stacks,
    std::vector<float>& vertices, std::vector<unsigned int>& indices)
{
    const float PI = 3.14159265359f;
    float x, y, z, xy;     // vertex position
    float nx, ny, nz;      // vertex normal

    // Generate vertices for each stack and sector
    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2 - i * PI / stacks;  // from +pi/2 to -pi/2
        xy = radius * cosf(stackAngle);                // r * cos(u)
        y = radius * sinf(stackAngle);                 // r * sin(u)

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * 2 * PI / sectors;    // from 0 to 2pi

            x = xy * cosf(sectorAngle);  // r * cos(u) * cos(v)
            z = xy * sinf(sectorAngle);  // r * cos(u) * sin(v)

            // Vertex position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Normal vector (normalized position)
            float length = sqrt(x * x + y * y + z * z);
            nx = x / length;
            ny = y / length;
            nz = z / length;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }

    // Generate indices for the sphere (two triangles per sector)
    int k1, k2;
    for (int i = 0; i < stacks; ++i) {
        k1 = i * (sectors + 1);      // beginning of current stack
        k2 = k1 + sectors + 1;       // beginning of next stack

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                // First triangle of the quad
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stacks - 1)) {
                // Second triangle of the quad
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}