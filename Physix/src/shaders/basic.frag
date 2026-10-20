// basic.frag -- fragment stage paired with basic.vert. Implements a
// straightforward per-fragment Phong lighting model (ambient + diffuse +
// specular, single point light) with a uniform per-draw-call base color and
// alpha, driven by objectColor/alpha set from C++ before each draw
// (Sphere::Render / Cuboid::Render).
//
// Moved out of an inline C++ string literal in Application.cpp;
// no shading logic changed in the move.
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
