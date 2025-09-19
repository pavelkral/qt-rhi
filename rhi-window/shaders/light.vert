#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_fragpos;


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
    vec4 worldPos = ubo.model * vec4(in_pos, 1.0);
    gl_Position = ubo.projection * ubo.view * worldPos;
    out_uv = in_uv;
    out_fragpos = worldPos.xyz;
    out_normal = mat3(transpose(inverse(ubo.model))) * in_normal;
}
