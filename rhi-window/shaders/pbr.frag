#version 450

layout(location = 0) in vec2 frag_uv;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec3 frag_pos;
layout(location = 3) in vec3 frag_tangent;
layout(location = 4) in vec3 frag_bitangent;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D tex_albedo;
layout(binding = 2) uniform sampler2D tex_normal;
layout(binding = 3) uniform sampler2D tex_metallic;
layout(binding = 4) uniform sampler2D tex_roughness;
layout(binding = 5) uniform sampler2D tex_ao;
layout(binding = 6) uniform sampler2D tex_height;
layout(binding = 7) uniform sampler2D tex_shadows;

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


// --- Shadow map helper ---
float shadowCalculationVulkan(vec3 normal, vec3 fragPos)
{
    // Transformace fragmentu do light-space
    vec4 fragPosLightSpace = ubo.lightSpace * vec4(fragPos, 1.0);

    // Perspektivní dělení
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Remap Z [-1,1] -> [0,1] pro Vulkan
    projCoords = projCoords * 0.5 + 0.5;

    // Pokud fragment je mimo shadow mapu, je osvětlen
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0)
    {
        return 1.0;
    }

    float currentDepth = projCoords.z;

    // Bias proti acne
    float bias = max(0.001 * (1.0 - dot(normalize(normal), normalize(ubo.lightPos.xyz - fragPos))),
                     0.001);

    // PCF (3x3)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(tex_shadows, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(tex_shadows, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}
void main()
{
    // --- TBN matice ---
    vec3 T = normalize(frag_tangent);
    vec3 B = normalize(frag_bitangent);
    vec3 N = normalize(frag_normal);
    mat3 TBN = mat3(T, B, N);

    // --- normála v world space ---
    vec3 N_world = normalize(TBN * (texture(tex_normal, frag_uv).xyz * 2.0 - 1.0));

    // --- stíny ---
    float shadowFactor = shadowCalculationVulkan(N_world, frag_pos);

    // --- směry ---
    vec3 V = normalize(ubo.camPos.xyz - frag_pos);
    vec3 L = normalize(ubo.lightPos.xyz - frag_pos);
    vec3 H = normalize(V + L);

    // --- textury ---
    vec3 albedo = texture(tex_albedo, frag_uv).rgb;
    float metallic = texture(tex_metallic, frag_uv).r;
    float roughness = texture(tex_roughness, frag_uv).r;
    float ao = texture(tex_ao, frag_uv).r;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

    float NdotL = max(dot(N_world, L), 0.0);
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 radiance = ubo.color.rgb * ubo.misc1.y;

    vec3 diffuse  = kD * albedo / 3.141592;
    vec3 specular = kS;
    vec3 Lo = (diffuse + specular) * radiance * NdotL;
    vec3 ambient = albedo * ao * 0.1;

    vec3 color = ambient + shadowFactor * Lo;

    // gamma korekce
    color = pow(color, vec3(1.0/2.2));

    out_color = vec4(color, 1.0);
}
