#version 450

layout(location = 0) in vec3 aPos;

layout(std140, binding = 0) uniform Ubuf {
    mat4 uProjection;
    mat4 uView;
} ubuf;

layout(location = 0) out vec3 vWorldPos; // <- location musí být zde

void main()
{
    vWorldPos = aPos;
    gl_Position = ubuf.uProjection * ubuf.uView * vec4(aPos, 1.0);
}
