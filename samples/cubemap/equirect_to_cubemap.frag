#version 450

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform sampler2D equirectMap;

void main() {
    vec3 dir = normalize(vDir);
    float phi = atan(dir.z, dir.x);
    float theta = asin(dir.y);

    vec2 uv;
    uv.x = (phi + 3.14159265) / (2.0 * 3.14159265);
    uv.y = (theta + 3.14159265 / 2.0) / 3.14159265;

    fragColor = texture(equirectMap, uv);
}
