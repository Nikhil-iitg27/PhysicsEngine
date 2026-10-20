#pragma once
#include <glm/glm.hpp>
#include "physics/RigidBody.h"
#include "render/RenderComponent.h"
#include "sphere_mesh.h"
#include "shader.h"

// A dynamic spherical entity in the scene: pairs a physics RigidBody with a
// RenderComponent. PhysicsWorld only ever touches `body`; the renderer only
// ever touches `render` (+ body.position). Sphere itself carries no
// simulation or rendering logic of its own -- it exists so call sites that
// want "one sphere" (spawning, depth-sorting for blending) can keep working
// with a single object -- this doesn't need a full
// ECS to achieve the physics/rendering separation.
class Sphere {
public:
    RigidBody body;
    RenderComponent render;

    Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position,
        glm::vec3 color = glm::vec3(0.0f, 0.0f, 1.0f), float transparency = 1.0f);

    void Render(const Shader& shader) const;
};
