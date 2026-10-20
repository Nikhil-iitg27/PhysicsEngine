#pragma once
#include <cstdint>
#include <vector>
#include "../camera.h"
#include "../sphere.h"

// Sphere-ray picking. Doesn't belong to Camera (no scene knowledge), Scene
// (shouldn't own ray-intersection math), or RigidBody (no screen-space
// knowledge) -- hence its own module.
//
// Returns the id (RigidBody::id) of the closest sphere the ray hits in front
// of its origin, or 0 (no id) if none. Linear scan, not spatial-structure-
// accelerated -- one ray per click is trivially cheap at this project's scale.
uint32_t PickClosestSphere(const Ray& ray, const std::vector<Sphere>& spheres);
