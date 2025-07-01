#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "sphere_mesh.h"
#include "shader.h"
#include "cuboid.h"

class Sphere {
public:
	// declare physical properties
	float mass;
	bool fixed = false;
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 acceleration;

	glm::vec3 color;
	float transparency;
	Sphere_mesh* mesh;
	
	Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position);
	Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position, glm::vec3 color);
	Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position, float transparency);
	Sphere(float mass, Sphere_mesh* mesh, glm::vec3 position, glm::vec3 color, float transparency);

	void SetVelocity(glm::vec3 velocity);
	void SetAcceleration(glm::vec3 Acceleration);
	void SetForce(glm::vec3 Force);

	void SetMesh(Sphere_mesh* mesh);
	void SetTransparency(float transparency);
	void SetColor(glm::vec3 color);
	void ToogleFixed();

	void Update(float deltaTime);
	void ProcessCuboidCollision(const std::vector<Cuboid>& cuboids);
	void ProcessSphereCollision(std::vector<Sphere>& spheres);

	void Render(const Shader& shader);
	glm::mat4 getModelMatrix() const;

private:
	glm::vec3 ComputeMomentum();
};