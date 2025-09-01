#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(std140, binding = 0) uniform UBO {
    mat4 u_mvp;
    mat4 u_model;
    mat3 u_normalMat;
    vec3 u_lightPos;
    float _pad0; // zarovnání
} ubo;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vWorldPos;
layout(location = 2) out vec2 vUV;

void main() {
    vec4 worldPos = ubo.u_model * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(ubo.u_normalMat * inNormal);
    vUV = inUV;
    gl_Position = ubo.u_mvp * vec4(inPos, 1.0);
}