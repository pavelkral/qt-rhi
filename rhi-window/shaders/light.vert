#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;

// Uniformní blok musí přesně odpovídat C++ struktuře Ubo
layout(std140, binding = 0) uniform Ubo {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpace;
    vec4 color;
    float opacity;
} ubo;

void main() {
    // Vypočítá finální pozici vertexu na obrazovce
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_pos, 1.0);

    // Pošle UV souřadnice do fragment shaderu
    out_uv = in_uv;
}
