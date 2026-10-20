// See SphereSpawner.h.
#include "SphereSpawner.h"

namespace {
    // Despite the "bound" names, this parameterizes a Gaussian (mean, stddev),
    // not a clamped interval -- preserved from the original behavior.
    float RandomGaussian(std::mt19937& gen, float mean, float stddev) {
        std::normal_distribution<float> dist(mean, stddev);
        return dist(gen);
    }

    float RandomUniform(std::mt19937& gen, float lowerBound, float upperBound) {
        std::uniform_real_distribution<float> dist(lowerBound, upperBound);
        return dist(gen);
    }
}

uint32_t SphereSpawner::SpawnOne(std::vector<Sphere>& spheres, Sphere_mesh* mesh,
    glm::vec3 position, glm::vec3 velocity, glm::vec3 color, float transparency) {
    Sphere newSphere(1.0f, mesh, position, color, transparency);
    newSphere.body.id = nextId++;
    newSphere.body.SetVelocity(velocity);
    spheres.push_back(newSphere);
    return newSphere.body.id;
}

void SphereSpawner::SpawnRandom(std::vector<Sphere>& spheres, Sphere_mesh* mesh, std::mt19937& gen) {
    glm::vec3 pos(
        RandomUniform(gen, -20.0f, 20.0f),
        RandomUniform(gen, -20.0f, 20.0f),
        RandomUniform(gen, -20.0f, 20.0f));
    glm::vec3 vel(
        RandomGaussian(gen, -5.0f, 5.0f),
        RandomGaussian(gen, -5.0f, 5.0f),
        RandomGaussian(gen, -5.0f, 5.0f));

    SpawnOne(spheres, mesh, pos, vel);
    spheres.back().body.SetAcceleration(glm::vec3(0.0f, 0.0f, -1.0f));
}

void SphereSpawner::Update(std::vector<Sphere>& spheres, Sphere_mesh* mesh, float deltaTime,
    float spawnInterval, size_t maxSpheres, std::mt19937& gen) {
    spawnTimer += deltaTime;

    while (spawnTimer >= spawnInterval && spheres.size() < maxSpheres) {
        SpawnRandom(spheres, mesh, gen);
        spawnTimer -= spawnInterval;
    }
}

void SphereSpawner::SpawnMany(std::vector<Sphere>& spheres, Sphere_mesh* mesh, size_t count, std::mt19937& gen) {
    for (size_t i = 0; i < count; i++) {
        SpawnRandom(spheres, mesh, gen);
    }
}
