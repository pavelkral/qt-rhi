#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_tangent; // xyz = tangent, w = handedness (+1 nebo -1)

layout(location = 0) out vec2 frag_uv;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec3 frag_pos;
layout(location = 3) out vec3 frag_tangent;
layout(location = 4) out vec3 frag_bitangent;

layout(std140, binding = 0) uniform Ubo {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpace;
    vec4 lightPos;
    vec4 color;
    vec4 camPos;
    float debugMode;
    float lightIntensity;
} ubo;

void main() {
    vec4 worldPos = ubo.model * vec4(in_pos, 1.0);
    frag_pos = worldPos.xyz;

    // normal in world-space
    vec3 N = normalize(mat3(ubo.model) * in_normal);
    frag_normal = N;

    // tangent in world-space (a handedness)
    vec3 T = normalize(mat3(ubo.model) * in_tangent.xyz);
    // ortogo tangent normal
    T = normalize(T - N * dot(N, T));

    // bitangent handedness (w)
    vec3 B = cross(N, T) * in_tangent.w;

    frag_tangent = T;
    frag_bitangent = B;

    frag_uv = in_uv;
    gl_Position = ubo.projection * ubo.view * worldPos;
}
