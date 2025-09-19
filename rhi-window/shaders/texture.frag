#version 450
layout(location = 0) in vec2 vUV;
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

void main() {
    //repeat texture
    vec2 repeatedUV = vUV * 2.0;
    mat4 mvp2 = ubo.model * ubo.view * ubo.projection;
    fragColor = texture(tex, repeatedUV);
}
