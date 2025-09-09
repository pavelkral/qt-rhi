#version 450

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

// Přístup ke stejnému uniformnímu bloku jako ve vertex shaderu
layout(std140, binding = 0) uniform Ubo {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpace;
    vec4 color;
    float opacity;
} ubo;

// Sampler pro texturu
layout(binding = 1) uniform sampler2D sampler_tex;

void main() {
    // Načtení barvy z textury
    vec4 texColor = texture(sampler_tex, in_uv);

    // Výsledná barva je kombinací barvy z textury a barvy z UBO
    // Můžete je libovolně kombinovat, zde je příklad násobení
    vec3 final_rgb = texColor.rgb * ubo.color.rgb;

    // Výsledná alfa je kombinací alfy z textury a průhlednosti z UBO
    float final_alpha = texColor.a * ubo.opacity;

    out_color = vec4(final_rgb, final_alpha);
}
