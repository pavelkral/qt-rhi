#version 450

layout(location = 0) in vec3 inPosition;

// Používáme tvůj stávající Ubo
layout(std140, binding = 0) uniform Ubo {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpace;
    vec4 lightPos;
    vec4 color;
    vec4 camPos;
    vec4 misc;
    vec4 misc1;
} ubo;

void main() {
    // Použijeme model + view + projection
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
}
