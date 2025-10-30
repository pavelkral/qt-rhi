#version 450

layout(location = 0) in vec3 position;
layout(location = 0) out vec3 vDir;

layout(set = 0, binding = 0) uniform UniformBuffer {
    mat4 mvp;
} ubuf;

void main() {
    vDir = position;
    gl_Position = ubuf.mvp * vec4(position, 1.0);
}