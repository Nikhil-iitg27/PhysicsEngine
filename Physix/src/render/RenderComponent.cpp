// See RenderComponent.h for what this class is; this file is the
// implementation.
#include "RenderComponent.h"
#include <glm/gtc/matrix_transform.hpp>

RenderComponent::RenderComponent(Sphere_mesh* mesh, glm::vec3 color, float transparency)
    : mesh(mesh), color(color), transparency(transparency) {
}

void RenderComponent::SetMesh(Sphere_mesh* mesh) {
    this->mesh = mesh;
}

void RenderComponent::SetColor(glm::vec3 color) {
    this->color = color;
}

void RenderComponent::SetTransparency(float transparency) {
    this->transparency = transparency;
}

void RenderComponent::Render(const Shader& shader, const glm::vec3& position) const {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    shader.setMat4("model", model);
    mesh->render();
}

glm::mat4 RenderComponent::getModelMatrix(const glm::vec3& position) const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(mesh->getRadius()));
    return model;
}
