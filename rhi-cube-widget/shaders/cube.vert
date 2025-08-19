#version 450

layout(std140, binding = 0) uniform buf { mat4 mvp; };
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec2 v_uv;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    v_uv = uv;
}
