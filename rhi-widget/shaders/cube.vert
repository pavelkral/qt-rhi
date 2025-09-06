#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

layout(location = 0) out vec2 vUV;

void main() {
    vUV = inUV;
    gl_Position = mvp * vec4(inPos, 1.0);
}
