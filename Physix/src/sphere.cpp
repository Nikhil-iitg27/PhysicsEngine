// See sphere.h for what Sphere is (a thin RigidBody + RenderComponent pair);
// this file is the implementation -- just the constructor (which snapshots
// the mesh's radius into the RigidBody so physics never depends on which
// mesh happens to be bound for rendering) and a Render() forwarder.
#include "sphere.h"

Sphere::Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position, glm::vec3 color, float transparency)
    : body(mass, mesh->getRadius(), position), render(mesh, color, transparency) {
}

void Sphere::Render(const Shader& shader) const {
    render.Render(shader, body.position);
}
