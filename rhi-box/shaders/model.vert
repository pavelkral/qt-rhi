#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(binding = 0) uniform UBuf {
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
    vec3 uLightPos;
    float uAmbientStrength;
    vec3 uLightColor;
    float uMetallicFactor;
    vec3 uCameraPos;
    float uSmoothnessFactor;
    vec3 uAlbedoColor;
    float pad0;
    int  uHasAlbedo;
    int  uHasNormal;
    int  uHasMetallic;
    int  uHasSmoothness;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out vec2 vUV;
layout(location = 1) out vec3 vNormal;

void main() {
    gl_Position = ubuf.uProj * ubuf.uView * ubuf.uModel * vec4(aPos, 1.0);
    vUV = aUV;
    vNormal = mat3(ubuf.uModel) * aNormal;
}
