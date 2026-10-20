// See Picker.h. Analytic ray-sphere intersection (unit-direction quadratic).
#include "Picker.h"
#include <limits>
#include <cmath>

uint32_t PickClosestSphere(const Ray& ray, const std::vector<Sphere>& spheres) {
    uint32_t closestId = 0;
    float closestDist = std::numeric_limits<float>::max();

    for (const Sphere& s : spheres) {
        glm::vec3 oc = ray.origin - s.body.position;
        float b = glm::dot(oc, ray.direction);
        float c = glm::dot(oc, oc) - s.body.radius * s.body.radius;
        float discriminant = b * b - c;
        if (discriminant < 0.0f) continue;

        float t = -b - std::sqrt(discriminant); // nearest intersection
        if (t < 0.0f) continue; // sphere is behind the ray origin

        if (t < closestDist) {
            closestDist = t;
            closestId = s.body.id;
        }
    }
    return closestId;
}
