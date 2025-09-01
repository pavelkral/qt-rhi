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
    vec3 V = normalize(-vWorldPos);
    vec3 R = reflect(-L, N);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(R, V), 0.0), 32.0);

    vec3 albedo = texture(u_tex, vUV).rgb;
    vec3 color = albedo * (0.15 + 0.85 * diff) + vec3(0.25) * spec;
    outColor = vec4(color, 1.0);
}