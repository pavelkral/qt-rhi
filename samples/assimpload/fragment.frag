#version 440

layout (location = 0) in vec2 texCoord;
layout (location = 0) out vec4 fragColor;

layout (binding = 1) uniform sampler2D base_color;
layout (binding = 2) uniform sampler2D normal;
layout (binding = 3) uniform sampler2D metallic;
layout (binding = 4) uniform sampler2D roughness;
layout (binding = 5) uniform sampler2D ao;

layout (std140, binding = 6) uniform buf
{
    vec4 base_color;// rgba
    float metallic;
    float roughness;
} factor;

void main()
{
    fragColor = texture(base_color, texCoord).bgra;
}
