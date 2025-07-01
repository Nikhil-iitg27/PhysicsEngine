#include "Cuboid.h"

Cuboid::Cuboid(Cuboid_mesh* mesh,
    const glm::vec3& position,
    const glm::vec3& rotation)
    : mesh(mesh), position(position), rotation(rotation)
{
    updateModelMatrix();
}

void Cuboid::Render(const Shader& shader) {
    shader.setMat4("model", getModelMatrix());
    mesh->render();
}

glm::mat4 Cuboid::getModelMatrix() const {
    return modelMatrix;
}

float Cuboid::getLength() const {
    return mesh->getLength();
}

float Cuboid::getBreadth() const {
    return mesh->getBreadth();
}

float Cuboid::getHeight() const {
    return mesh->getHeight();
}

void Cuboid::setPosition(const glm::vec3& newPosition) {
    position = newPosition;
    updateModelMatrix();
}

void Cuboid::setRotation(const glm::vec3& newRotation) {
    rotation = newRotation;
    updateModelMatrix();
}

void Cuboid::updateModelMatrix() {
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    // Apply rotations about X, Y, then Z axes.
    modelMatrix = glm::rotate(modelMatrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
}

std::vector<Face> Cuboid::getSurfacePlanes() const {
    std::vector<Face> faces;

    // Retrieve half-dimensions from the mesh.
    float hx = getLength() * 0.5f;
    float hy = getHeight() * 0.5f;
    float hz = getBreadth() * 0.5f;

    // Define local face data (local normal and offset from the center).
    struct LocalFace {
        glm::vec3 localNormal;
        glm::vec3 localOffset;
    };

    // Convention:
    // Front: +Z, Back: -Z, Right: +X, Left: -X, Top: +Y, Bottom: -Y.
    std::vector<LocalFace> localFaces = {
        { glm::vec3(0.0f, 0.0f, 1.0f),  glm::vec3(0.0f, 0.0f, hz) },
        { glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, -hz) },
        { glm::vec3(1.0f, 0.0f, 0.0f),  glm::vec3(hx, 0.0f, 0.0f) },
        { glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(-hx, 0.0f, 0.0f) },
        { glm::vec3(0.0f, 1.0f, 0.0f),  glm::vec3(0.0f, hy, 0.0f) },
        { glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -hy, 0.0f) }
    };

    // For each local face, compute the corresponding world-space plane.
    for (const LocalFace& lf : localFaces) {
        // Transform the local offset into world space.
        glm::vec4 worldPoint4 = modelMatrix * glm::vec4(lf.localOffset, 1.0f);
        glm::vec3 worldPoint = glm::vec3(worldPoint4);

        // Transform the local normal into world space.
        glm::vec3 worldNormal = glm::normalize(glm::mat3(modelMatrix) * lf.localNormal);

        faces.push_back({ worldPoint, worldNormal });
    }

    return faces;
}
