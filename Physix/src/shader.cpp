// See shader.h for what Shader is (a GLSL compile/link wrapper); this file
// is the implementation, including FromFiles' executable-relative file
// loading (see LoadTextFile/GetExecutableDirectory below).
#include "Shader.h"
#include <iostream>
#include <sstream>
#include <fstream>
// Shader.h includes glad/glad.h, which defines APIENTRY unconditionally;
// <Windows.h> defines it again, so undef first to avoid a redefinition
// warning (both definitions are __stdcall -- this doesn't change anything).
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <Windows.h>

namespace {
    // Directory the running .exe lives in, with a trailing slash -- used so
    // shader file paths are resolved relative to the executable, not to
    // whatever the current working directory happens to be at launch.
    std::string GetExecutableDirectory() {
        char path[MAX_PATH];
        DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string fullPath(path, length);
        size_t lastSlash = fullPath.find_last_of("\\/");
        return (lastSlash == std::string::npos) ? "" : fullPath.substr(0, lastSlash + 1);
    }

    std::string LoadTextFile(const std::string& relativePath) {
        std::string fullPath = GetExecutableDirectory() + relativePath;
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open shader file: " << fullPath << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}

// Helper function to compile a shader
static unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment")
            << " Shader compilation error:\n" << infoLog << std::endl;
    }
    return shader;
}

Shader::Shader(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    // Create shader program and link
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    // Check for linking errors
    int success;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(ID, 512, nullptr, infoLog);
        std::cerr << "Shader linking error:\n" << infoLog << std::endl;
    }

    // Clean up shaders (they are linked into the program now)
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader Shader::FromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource = LoadTextFile(vertexPath);
    std::string fragmentSource = LoadTextFile(fragmentPath);
    return Shader(vertexSource.c_str(), fragmentSource.c_str());
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &vec[0]);
}
