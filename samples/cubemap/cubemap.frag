#version 450

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform samplerCube skyboxTex;

void main() {
    fragColor = texture(skyboxTex, normalize(vDir));
}