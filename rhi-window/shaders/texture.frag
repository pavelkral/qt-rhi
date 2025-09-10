#version 450
layout(location = 0) in vec2 vUV;
layout(binding = 1) uniform sampler2D tex;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float opacity;
};
void main() {
    fragColor = texture(tex, vUV);
}
