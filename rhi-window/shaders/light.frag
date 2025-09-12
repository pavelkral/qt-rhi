#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_fragpos;
layout(binding = 1) uniform sampler2D sampler_tex;

layout(location = 0) out vec4 out_color;

layout(std140, binding = 0) uniform Ubo {
    mat4 mvp;
    vec4 opacity;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpace;
    vec4 lightPos;
    vec4 color;
} ubo;

float ambientStrength = 0.8;

void main() {

    vec2 repeatedUV = in_uv * 2.0;
    vec3 norm = normalize(in_normal);
    vec3 lightDir = normalize(ubo.lightPos.xyz - in_fragpos);

    // Ambient
   // vec3 ambient = 0.3 * ubo.color.rgb;
    vec3 ambient = ambientStrength * ubo.color.rgb * texture(sampler_tex, repeatedUV).rgb;
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.color.rgb;

    // Specular
    vec3 viewDir = normalize(-in_fragpos); // kamera v (0,0,0)
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.3 * spec * vec3(1.0);

    vec4 texColor = texture(sampler_tex,repeatedUV);
    vec3 lighting = (ambient + diffuse + specular) * texColor.rgb;
    float alpha = texColor.a ;

    out_color = vec4(lighting, alpha);
}
