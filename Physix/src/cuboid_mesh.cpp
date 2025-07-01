#include "cuboid_mesh.h"
#include <vector>

Cuboid_mesh::Cuboid_mesh(float length, float breadth, float height)
    : VAO(0), VBO(0), EBO(0), indexCount(0),
    length(length), breadth(breadth), height(height)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateCuboidMesh(length, breadth, height, vertices, indices);
    indexCount = static_cast<unsigned int>(indices.size());

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create and fill VBO.
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Create and fill EBO.
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Set vertex attributes: Position (location 0) and Normal (location 1).
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

Cuboid_mesh::~Cuboid_mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Cuboid_mesh::render() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

float Cuboid_mesh::getLength() const {
    return length;
}

float Cuboid_mesh::getBreadth() const {
    return breadth;
}

float Cuboid_mesh::getHeight() const {
    return height;
}

void Cuboid_mesh::generateCuboidMesh(float length, float breadth, float height,
    std::vector<float>& vertices,
    std::vector<unsigned int>& indices)
{
    // Compute half-dimensions (for centering the cuboid at the origin).
    float hx = length * 0.5f;
    float hy = height * 0.5f;
    float hz = breadth * 0.5f;

    // Define the vertices for each face.
    // For each face, we define 4 vertices (with positions and a common normal).
    // Front face (normal: (0, 0, 1))
    glm::vec3 frontNormal(0.0f, 0.0f, 1.0f);
    glm::vec3 front1(-hx, -hy, hz);
    glm::vec3 front2(hx, -hy, hz);
    glm::vec3 front3(hx, hy, hz);
    glm::vec3 front4(-hx, hy, hz);

    // Back face (normal: (0, 0, -1))
    glm::vec3 backNormal(0.0f, 0.0f, -1.0f);
    glm::vec3 back1(hx, -hy, -hz);
    glm::vec3 back2(-hx, -hy, -hz);
    glm::vec3 back3(-hx, hy, -hz);
    glm::vec3 back4(hx, hy, -hz);

    // Left face (normal: (-1, 0, 0))
    glm::vec3 leftNormal(-1.0f, 0.0f, 0.0f);
    glm::vec3 left1(-hx, -hy, -hz);
    glm::vec3 left2(-hx, -hy, hz);
    glm::vec3 left3(-hx, hy, hz);
    glm::vec3 left4(-hx, hy, -hz);

    // Right face (normal: (1, 0, 0))
    glm::vec3 rightNormal(1.0f, 0.0f, 0.0f);
    glm::vec3 right1(hx, -hy, hz);
    glm::vec3 right2(hx, -hy, -hz);
    glm::vec3 right3(hx, hy, -hz);
    glm::vec3 right4(hx, hy, hz);

    // Top face (normal: (0, 1, 0))
    glm::vec3 topNormal(0.0f, 1.0f, 0.0f);
    glm::vec3 top1(-hx, hy, hz);
    glm::vec3 top2(hx, hy, hz);
    glm::vec3 top3(hx, hy, -hz);
    glm::vec3 top4(-hx, hy, -hz);

    // Bottom face (normal: (0, -1, 0))
    glm::vec3 bottomNormal(0.0f, -1.0f, 0.0f);
    glm::vec3 bottom1(-hx, -hy, -hz);
    glm::vec3 bottom2(hx, -hy, -hz);
    glm::vec3 bottom3(hx, -hy, hz);
    glm::vec3 bottom4(-hx, -hy, hz);

    // Helper lambda to push face data.
    auto pushFace = [&](const glm::vec3& a, const glm::vec3& b,
        const glm::vec3& c, const glm::vec3& d,
        const glm::vec3& normal)
        {
            // Each vertex: position (3 floats) then normal (3 floats)
            vertices.insert(vertices.end(), { a.x, a.y, a.z, normal.x, normal.y, normal.z });
            vertices.insert(vertices.end(), { b.x, b.y, b.z, normal.x, normal.y, normal.z });
            vertices.insert(vertices.end(), { c.x, c.y, c.z, normal.x, normal.y, normal.z });
            vertices.insert(vertices.end(), { d.x, d.y, d.z, normal.x, normal.y, normal.z });
        };

    pushFace(front1, front2, front3, front4, frontNormal);
    pushFace(back1, back2, back3, back4, backNormal);
    pushFace(left1, left2, left3, left4, leftNormal);
    pushFace(right1, right2, right3, right4, rightNormal);
    pushFace(top1, top2, top3, top4, topNormal);
    pushFace(bottom1, bottom2, bottom3, bottom4, bottomNormal);

    // For each face, create two triangles (6 indices).
    for (unsigned int i = 0; i < 6; ++i) {
        unsigned int offset = i * 4;
        indices.push_back(offset + 0);
        indices.push_back(offset + 1);
        indices.push_back(offset + 2);
        indices.push_back(offset + 0);
        indices.push_back(offset + 2);
        indices.push_back(offset + 3);
    }
}
