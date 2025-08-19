#version 450

layout(std140, binding = 0) uniform buf {
    mat4 mvp[6]; // pole pro 6 krychlí
};

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec2 vUV;

void main() {
    vUV = inUV;
    gl_Position = mvp[gl_InstanceIndex] * vec4(inPos, 1.0);
}
