#version 440

layout(binding = 1) uniform sampler2D tex;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec4 opacity;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpace;
    vec4 lightPos;
    vec4 camPos;
    vec4 color;
};

vec3 mcolor = vec3(1.0, 0.0, 0.0);

void main()
{

    fragColor = vec4(mcolor * opacity.x, opacity.x);
}
