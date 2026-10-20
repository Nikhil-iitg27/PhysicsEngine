// basic.vert -- vertex stage of the single shader program this project uses
// to draw both spheres and the room's walls. Transforms each vertex from
// model space to clip space and passes world-space position/normal through
// to the fragment stage for Phong-style lighting (see basic.frag).
//
// Moved out of an inline C++ string literal in Application.cpp so it can be
// edited as GLSL with normal tooling instead of C++; no shading
// logic changed in the move.
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
