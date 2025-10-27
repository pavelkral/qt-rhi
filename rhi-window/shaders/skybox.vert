#version 450

layout(location = 0) in vec3 inPos;       // Tvůj HdriVertex.pos
layout(location = 0) out vec3 vDir;      // Směrový vektor pro fragment shader

layout(set = 0, binding = 0) uniform SkyUbo {
    mat4 projection;
    mat4 view;
} ubo;

void main()
{
    // Odstraň translaci kamery, aby zůstala skybox "stacionární"
    mat4 viewNoTrans = ubo.view;
    viewNoTrans[3] = vec4(0,0,0,1);

    vDir = inPos; // směr pro sampling cubemapy
    gl_Position = ubo.projection * viewNoTrans * vec4(inPos, 1.0);
}
