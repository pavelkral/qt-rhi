#version 450
layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vWorldPos;
layout(location = 2) in vec2 vUV;

layout(binding = 1) uniform sampler2D u_tex;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform UBO {
    mat4 u_mvp;
    mat4 u_model;
    mat3 u_normalMat;
    vec3 u_lightPos;
    float _pad0;
} ubo;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(ubo.u_lightPos - vWorldPos);
    float ndotl = max(dot(N, L), 0.0);
    float levels = 4.0;
    float toon = floor(ndotl * levels) / (levels - 1.0);
    vec3 albedo = texture(u_tex, vUV).rgb;
    vec3 col = albedo * (0.15 + 0.85 * toon);
    outColor = vec4(col, 1.0);
}