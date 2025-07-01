#include <glm/gtc/matrix_transform.hpp>
#include "sphere.h"
#include <iostream>
#include <cmath>

//CONSTRUCTORS
Sphere::Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position)
    : mass(mass), position(position), velocity(0.0f), acceleration(0.0f), mesh(mesh), color(glm::vec3(0.0f, 0.0f, 1.0f)), transparency(1.0f){ 
};

Sphere::Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position, glm::vec3 color)
    : mass(mass), position(position), velocity(0.0f), acceleration(0.0f), mesh(mesh), color(color), transparency(1.0f) {
};

Sphere::Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position, float transparency)
    : mass(mass), position(position), velocity(0.0f), acceleration(0.0f), mesh(mesh), color(glm::vec3(0.0f, 0.0f, 1.0f)), transparency(transparency) {
};

Sphere::Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position, glm::vec3 color, float transparency)
    : mass(mass), position(position), velocity(0.0f), acceleration(0.0f), mesh(mesh), color(color), transparency(transparency) {
};

void Sphere::SetVelocity(glm::vec3 velocity){
    this->velocity = velocity;
}

void Sphere::SetAcceleration(glm::vec3 Acceleration){
    this->acceleration = Acceleration;
}

void Sphere::SetForce(glm::vec3 Force) {
    this->SetAcceleration(acceleration + (Force * (1 / mass)));
}

void Sphere::ToogleFixed(){
    this->fixed = !this->fixed;
}
void Sphere::Update(float deltaTime){
    if (this->fixed)return;
    // Update velocity with acceleration
    velocity += acceleration * deltaTime;
    // Update position with velocity
    position += velocity * deltaTime;
}

void Sphere::Render(const Shader& shader) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    shader.setMat4("model", model);
    mesh->render();
}

glm::mat4 Sphere::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(mesh->getRadius()));
    return model;
}

void Sphere::SetMesh(Sphere_mesh* mesh) {
    this->mesh = mesh;
}
void Sphere::SetTransparency(float transparency) {
    this->transparency = transparency;
}
void Sphere::SetColor(glm::vec3 color) {
    this->color = color;
}

void Sphere::ProcessCuboidCollision(const std::vector<Cuboid>& cuboids) {
    for (const Cuboid& cuboid : cuboids) {
        glm::mat4 model = cuboid.getModelMatrix();
        glm::mat4 invModel = glm::inverse(model);

        float hx = cuboid.getLength() * 0.5f;
        float hy = cuboid.getHeight() * 0.5f;
        float hz = cuboid.getBreadth() * 0.5f;

        glm::vec4 localCenter4 = invModel * glm::vec4(position, 1.0f);
        glm::vec3 localCenter = glm::vec3(localCenter4) / localCenter4.w;

        std::vector<Face> faces = cuboid.getSurfacePlanes();

        auto inBounds = [&](int i) {
            switch (i) {
            case 0: return (localCenter.x >= -hx && localCenter.x <= hx &&
                localCenter.y >= -hy && localCenter.y <= hy);
            case 1: return (localCenter.x >= -hx && localCenter.x <= hx &&
                localCenter.y >= -hy && localCenter.y <= hy);
            case 2: return (localCenter.z >= -hz && localCenter.z <= hz &&
                localCenter.y >= -hy && localCenter.y <= hy);
            case 3: return (localCenter.z >= -hz && localCenter.z <= hz &&
                localCenter.y >= -hy && localCenter.y <= hy);
            case 4: return (localCenter.x >= -hx && localCenter.x <= hx &&
                localCenter.z >= -hz && localCenter.z <= hz);
            case 5: return (localCenter.x >= -hx && localCenter.x <= hx &&
                localCenter.z >= -hz && localCenter.z <= hz);
            default: return false;
            }
            };

        for (size_t i = 0; i < faces.size(); i++) {
            if (!inBounds(static_cast<int>(i))) continue;
            const Face& face = faces[i];
            float distance = glm::dot(position - face.point, face.normal);
            float restitution = 1.0f;
            float r = mesh->getRadius();
            if (std::abs(distance) < r) {
                float pen = r - std::abs(distance) + 0.5f;
                position += (distance < 0 ? -pen : pen) * face.normal;
                velocity -= (1.0f + restitution) * glm::dot(velocity, face.normal) * face.normal; 
            }
        }
    }
}

void Sphere::ProcessSphereCollision(std::vector<Sphere>& spheres) {
    for (Sphere& other : spheres) {
        if (&other == this) continue;

        glm::vec3 diff = position - other.position;
        float dist = glm::length(diff);
        float r1 = mesh->getRadius();
        float r2 = other.mesh->getRadius();

        float minDist = r1 + r2;

        if (dist < minDist && dist > 0.0f) {
            glm::vec3 normal = glm::normalize(diff);
            float penetration = minDist - dist;

            // Resolve penetration (split push)
            position += 0.5f * penetration * normal;
            other.position -= 0.5f * penetration * normal;

            // Relative velocity
            glm::vec3 relativeVel = velocity - other.velocity;
            float velAlongNormal = glm::dot(relativeVel, normal);

            if (velAlongNormal > 0.0f) continue; // Already separating

            // Compute impulse scalar (assuming perfectly elastic collision: restitution = 1.0)
            float restitution = 1.0f;
            float invMass1 = 1.0f / mass;
            float invMass2 = 1.0f / other.mass;

            float j = -(1 + restitution) * velAlongNormal;
            j /= invMass1 + invMass2;

            glm::vec3 impulse = j * normal;

            // Apply impulses
            velocity += impulse * invMass1;
            other.velocity -= impulse * invMass2;
        }
    }
}
glm::vec3 Sphere::ComputeMomentum(){
    return mass * velocity;
}
