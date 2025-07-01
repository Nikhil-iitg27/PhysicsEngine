// main.cpp
#include <glad/glad.h>      // Must be included before GLFW
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <random>
#include <thread>
#include <functional>

#include "sphere_mesh.h"
#include "sphere.h"
#include "shader.h"
#include "Cuboid_mesh.h"  // For the floor
#include "Cuboid.h"       // For the floor
#include "camera.h"

// Window dimensions
unsigned int SCR_WIDTH = 1600;
unsigned int SCR_HEIGHT = 1200;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

GLFWwindow* initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return nullptr;
    }
    // Configure GLFW for OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Application", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    // Register callback for window resize.
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load GLAD after creating the OpenGL context.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return nullptr;
    }
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    return window;
}

float normal(std::mt19937* gen, float left_bound, float right_bound) {
    std::normal_distribution<float> random(left_bound, right_bound); return random(*gen);
}

float uniform(std::mt19937* gen, float left_bound, float right_bound) {
    std::uniform_real_distribution<float> random(left_bound, right_bound); return random(*gen);
}

Sphere_mesh get_sphere_mesh(float radius, int sectors, int stacks) {
   return Sphere_mesh(radius, sectors, stacks);
}

Cuboid_mesh get_cuboid_mesh(float height, float width, float thickness) {
    return Cuboid_mesh(height, width, thickness);
}

int sizz = 0;
void SphereSpawner(std::vector<Sphere>& spheres, Sphere_mesh* sphereMesh, float deltaTime, float spawnInterval, size_t maxSpheres, std::mt19937* gen) {

    static float startTimer = 0.0f; startTimer += deltaTime;
    static float currentTimer = 0.0f; currentTimer += deltaTime;
    static float spawnTimer = 0.0f; spawnTimer += deltaTime;

    if (currentTimer >= startTimer && spawnTimer >= spawnInterval && spheres.size() < maxSpheres) {
        while (spawnTimer >= spawnInterval) {
            sizz++;
            glm::vec3 pos(uniform(gen, -20.0f, 20.0f), uniform(gen, -20.0f, 20.0f), uniform(gen, -20.0f, 20.0f));
            Sphere newSphere(1.0, sphereMesh, pos, glm::vec3(0.0f, 0.0f, 1.0f), 1.0f);
            glm::vec3 vel(normal(gen, -5.0f, 5.0f), normal(gen, -5.0f, 5.0f), normal(gen, -5.0f, 5.0f));
            newSphere.SetVelocity(vel); newSphere.SetAcceleration(glm::vec3(0.0f, 0.0f, -1.0f));
            spheres.push_back(newSphere);
            spawnTimer -= spawnInterval;
        }
    }
}

void WallSpawner(std::vector<Cuboid>& walls, Cuboid_mesh* wallMesh) {
    walls.clear();

    float regionSize = 60.0f;
    float halfSize = regionSize / 2.0f;
    float thickness = 1.0f;

    // Floor (XZ plane, no rotation)
    walls.emplace_back(wallMesh,
        glm::vec3(0.0f, -halfSize - thickness / 2.0f, 0.0f),
        glm::vec3(0.0f));

    // Ceiling (XZ plane, no rotation)
    walls.emplace_back(wallMesh,
        glm::vec3(0.0f, halfSize + thickness / 2.0f, 0.0f),
        glm::vec3(0.0f));

    // Left Wall (YZ plane) ? rotate +90° around Y
    walls.emplace_back(wallMesh,
        glm::vec3(-halfSize - thickness / 2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, glm::radians(90.0f)));

    // Right Wall (YZ plane) ? rotate -90° around Y
    walls.emplace_back(wallMesh,
        glm::vec3(halfSize + thickness / 2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, glm::radians(-90.0f)));

    // Back Wall (XY plane) ? rotate -90° around X
    walls.emplace_back(wallMesh,
        glm::vec3(0.0f, 0.0f, -halfSize - thickness / 2.0f),
        glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f));

    // Front Wall (XY plane) ? rotate +90° around X
    walls.emplace_back(wallMesh,
        glm::vec3(0.0f, 0.0f, halfSize + thickness / 2.0f),
        glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
}



void ProcessCollisions(std::vector<Sphere>& spheres, std::vector<Cuboid>& walls, float deltaTime, int iterations, int thread_id) {
    size_t siz = spheres.size(); int batch_size = siz / 10;
    int left = batch_size * thread_id;
    int right = (thread_id == 9) ? siz : batch_size * (thread_id + 1);

    for (int i = 0; i < iterations; i++) {
        for (int j = left; j < right; j++) {
            spheres[j].ProcessSphereCollision(spheres);
            spheres[j].ProcessCuboidCollision(walls);
            spheres[j].Update(deltaTime / iterations);
            spheres[j].SetAcceleration(-10.0f * spheres[j].position);
        }
    }
}

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;    // Vertex position
layout (location = 1) in vec3 aNormal; // Vertex normal

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightPos;   // Light position
uniform vec3 viewPos;    // Camera position
uniform vec3 objectColor;
uniform float alpha;

void main() {
    // Ambient lighting
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * objectColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * objectColor;

    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, alpha);
}
)";

int main() {
    GLFWwindow* window = initialize(); if (window == nullptr)return -1;

    // Build shader program.
    Shader shader(vertexShaderSource, fragmentShaderSource);
    Camera camera(glm::vec3(0.0f, 0.0f, 100.0f), glm::vec3(0.0, 0.0f, -1.0f), 0);

    // Create meshes for sphere and wall
    Sphere_mesh sphereMesh_high = get_sphere_mesh(1.0f, 25, 15);
    Sphere_mesh sphereMesh_low = get_sphere_mesh(1.0f, 10, 5);
    Cuboid_mesh wallMesh(60.0f, 60.0f, 1.0f);

    // Set up random number generators
    std::random_device rd;
    std::mt19937 gen(rd());

    // Create vector to hold spheres.
    std::vector<Sphere> spheres;
    std::vector<Cuboid> walls;
    WallSpawner(walls, &wallMesh);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    float lastFrame = 0.0f;
    float deltaTime = 0.0f;
    float speed = 5.0f;

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        SphereSpawner(spheres, &sphereMesh_high, deltaTime, 0.5f, 50, &gen);

        // Clear color and depth buffers.
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader and set view/projection matrices.
        shader.use(); camera.detectKeyPress(window, deltaTime);
        float aspect = static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(aspect);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // Set lighting uniforms.
        shader.setVec3("lightPos", camera.getPosition());
        shader.setVec3("viewPos", glm::vec3(0.0f, 0.0f, 3.0f));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Now render spheres (after transparent objects if blending is enabled)
        std::sort(spheres.begin(), spheres.end(), [&camera](const Sphere& a, const Sphere& b) {
            return camera.distanceFromCameraPlane(a.position) > camera.distanceFromCameraPlane(b.position);
            });
        for (Sphere& s : spheres) {
            /*if (camera.distanceFromCameraPlane(s.position) > 50.0f)s.SetMesh(&sphereMesh_low);
            else s.SetMesh(&sphereMesh_high);*/
            
            shader.setVec3("objectColor", s.color);
            shader.setFloat("alpha", s.transparency);
            s.Render(shader);
        }

        for (Cuboid& wall : walls) {
            shader.setVec3("objectColor", glm::vec3(0.5f));
            shader.setFloat("alpha", 0.0f);
            wall.Render(shader);
        }
        
        int iterations = 5;
        int num_threads = 10;
        std::vector<std::thread> threads;
        std::vector<std::vector<Sphere>> batches;

        for (int thread_id = 0; thread_id < num_threads; thread_id++) {
            threads.emplace_back(ProcessCollisions, std::ref(spheres), std::ref(walls), deltaTime, iterations, thread_id);
        }

        for (auto& t : threads) {
            t.join();
        }
        
        std::cout << "No of Objects: " << sizz << " & " <<  "Current FPS: " << (int)(1 / deltaTime) << "\n";
        // Swap buffers and poll IO events.
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
