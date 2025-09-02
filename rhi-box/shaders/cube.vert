#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(std140, binding = 0) uniform UBO {
    mat4 u_mvp;
    mat4 u_model;
    mat4 u_normalMat;
    vec4 u_lightPos;
};

// explicitní location pro outputy
layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vPos;
layout(location = 2) out vec2 vUV;

void main()
{
    gl_Position = u_mvp * vec4(inPos, 1.0);
    vPos = (u_model * vec4(inPos, 1.0)).xyz;
    vNormal = normalize((u_normalMat * vec4(inNormal, 0.0)).xyz);
    vUV = inUV;
}
