#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vPos;
layout(location = 2) in vec2 vUV;

layout(std140, binding = 0) uniform UBO {
    mat4 u_mvp;
    mat4 u_model;
    mat4 u_normalMat;
    vec4 u_lightPos;
};

layout(binding = 1) uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(u_lightPos.xyz - vPos);
    float diff = dot(N, L);

    float shade = diff > 0.75 ? 1.0 :
                  diff > 0.5  ? 0.7 :
                  diff > 0.25 ? 0.4 : 0.1;

    vec3 texCol = texture(tex, vUV).rgb;
    fragColor = vec4(texCol * shade, 1.0);
}
