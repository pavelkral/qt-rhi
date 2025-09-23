#version 450

layout(location = 0) in vec3 in_pos;
 // xyz = tangent, w = handedness (+1 nebo -1)

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


void main()
{
    gl_Position = ubo.lightSpace * ubo.model * vec4(in_pos, 1.0);
}
