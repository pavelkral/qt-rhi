#version 440

layout(location = 0) in vec3 v_color;
layout(binding = 1) uniform sampler2D tex;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

void main()
{

    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
