#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 inUV;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec4 opacity;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpace;
    vec4 lightPos;
    vec4 camPos;
    vec4 color;
};

layout(location = 0) out vec2 vUV;

void main() {
    vUV = inUV;
    gl_Position = mvp * vec4(inPos, 1.0);
}
