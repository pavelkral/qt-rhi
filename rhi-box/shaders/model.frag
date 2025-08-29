#version 450

layout(binding = 0) uniform UBuf {
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
    vec3 uLightPos;     float uAmbientStrength;
    vec3 uLightColor;   float uMetallicFactor;
    vec3 uCameraPos;    float uSmoothnessFactor;
    vec3 uAlbedoColor;  float pad0;
    int  uHasAlbedo;
    int  uHasNormal;
    int  uHasMetallic;
    int  uHasSmoothness;
} ubuf;

layout(binding = 1) uniform sampler2D uAlbedo;
layout(binding = 2) uniform sampler2D uNormal;
layout(binding = 3) uniform sampler2D uMetallic;
layout(binding = 4) uniform sampler2D uSmoothness;

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec3 vNormal;
layout(location = 0) out vec4 fragColor;

void main() {
    vec3 albedo = ubuf.uHasAlbedo != 0 ? texture(uAlbedo, vUV).rgb : ubuf.uAlbedoColor;
    vec3 N = normalize(vNormal);
    vec3 L = normalize(ubuf.uLightPos - (vec3(0.0)));
    float diff = max(dot(N, L), 0.0);
    vec3 color = (ubuf.uAmbientStrength + diff) * albedo * ubuf.uLightColor;
    fragColor = vec4(color, 1.0);
}
