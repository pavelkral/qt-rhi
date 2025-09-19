#version 440

layout(binding = 1) uniform sampler2D tex;
layout(location = 0) out vec4 fragColor;

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

vec3 mcolor = vec3(1.0, 0.0, 0.0);

void main()
{

    fragColor = vec4(mcolor * ubo.misc.w, ubo.misc.x);
}
