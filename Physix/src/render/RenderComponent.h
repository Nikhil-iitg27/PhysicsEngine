#pragma once
#include <glm/glm.hpp>
#include "../sphere_mesh.h"
#include "../shader.h"

// Pure rendering data for a sphere-shaped entity: which mesh to draw it with,
// its color and transparency, and the draw call itself. Holds no physical
// state of its own -- callers (Sphere, or eventually a Scene) supply the
// world position to render at each frame.
class RenderComponent {
public:
    Sphere_mesh* mesh;
    glm::vec3 color;
    float transparency;

    RenderComponent(Sphere_mesh* mesh, glm::vec3 color = glm::vec3(0.0f, 0.0f, 1.0f), float transparency = 1.0f);

    void SetMesh(Sphere_mesh* mesh);
    void SetColor(glm::vec3 color);
    void SetTransparency(float transparency);

    void Render(const Shader& shader, const glm::vec3& position) const;
    glm::mat4 getModelMatrix(const glm::vec3& position) const;
};
