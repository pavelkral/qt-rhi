
#version 450

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform samplerCube skybox;

void main()
{
    vec3 dir = normalize(vDir);
    vec3 color = texture(skybox, dir).rgb;

    // Jednoduchý tonemap pro LDR HDR mix (nepovinné)
    color = color / (color + vec3(1.0));

    fragColor = vec4(color, 1.0);
}
