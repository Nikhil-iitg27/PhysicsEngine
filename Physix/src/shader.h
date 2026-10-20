#ifndef SHADER_H
#define SHADER_H

#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Shader: minimal GLSL program wrapper -- compiles a vertex + fragment shader,
// links them into a program (ID), and exposes typed uniform setters. This
// project uses exactly one shader program (see src/shaders/basic.vert/.frag)
// for both spheres and walls, implementing per-fragment Phong lighting
// (ambient + diffuse + specular, single point light).
class Shader {
public:
    // The program ID
    unsigned int ID;

    // Constructor that builds the shader program from source strings
    Shader(const char* vertexSource, const char* fragmentSource);

    // Builds a shader program by reading GLSL source from two files and
    // compiling/linking them, same as the string-source constructor above.
    // Paths are resolved relative to the running executable's own directory
    // (not the process's current working directory), so this works
    // regardless of how the exe is launched -- double-clicked, run from a
    // shell in some other directory, or started by Visual Studio's debugger.
    static Shader FromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    // Use/activate the shader program
    void use() const;

    // Utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& vec) const;
};

#endif // SHADER_H
