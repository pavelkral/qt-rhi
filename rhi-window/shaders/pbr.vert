#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec3 in_tangent;

layout(location = 0) out vec2 frag_uv;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec3 frag_pos;
layout(location = 3) out mat3 frag_tbn;

layout(std140, binding = 0) uniform Ubo {
    mat4 mvp;
    vec4 opacity;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpace;
    vec4 lightPos;
    vec4 camPos;
    vec4 color;
} ubo;

void main() {
    // Pozice ve světovém prostoru
    vec4 worldPos = ubo.model * vec4(in_pos, 1.0);
    frag_pos = worldPos.xyz;

    // Normál ve světovém prostoru
    frag_normal = normalize(mat3(ubo.model) * in_normal);

    // Tangent ve světovém prostoru
    vec3 T = normalize(mat3(ubo.model) * in_tangent);
    vec3 N = normalize(frag_normal);
    vec3 B = normalize(cross(N, T));

    frag_tbn = mat3(T, B, N);

    frag_uv = in_uv;

    gl_Position = ubo.mvp * vec4(in_pos, 1.0);
}
