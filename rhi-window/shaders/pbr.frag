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


// --- PBR a Shadow map pomocné funkce ---

const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001);
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

// Smith's method for Geometry
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// --- Shadow map helper ---
float shadowCalculationVulkan(vec3 normal, vec3 fragPos)
{
    vec4 fragPosLightSpace = ubo.lightSpace * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) {
        return 0.0; // Není ve stínu
    }

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, normalize(ubo.lightPos.xyz - fragPos))), 0.001);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(tex_shadows, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(tex_shadows, projCoords.xy + vec2(x, y) * texelSize).r;
            // Pokud je fragment dál než nejbližší povrch v mapě, je ve stínu
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow; // Vrací 0.0 (osvětleno) až 1.0 (zastíněno)
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

    // --- PBR výpočty ---
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float NdotL = max(dot(N_world, L), 0.0);
    float NdotV = max(dot(N_world, V), 0.0);
    float NDF = DistributionGGX(N_world, H, roughness);
    float G   = GeometrySmith(N_world, V, L, roughness);
    vec3  F   = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - max(dot(H, V), 0.0), 0.0, 1.0), 5.0);
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular     = numerator / denominator;
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    // --- finální osvětlení ---
    vec3 radiance = ubo.color.rgb * ubo.misc1.y;
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = albedo * ao * 0.1;

    // ZDE JE KLÍČOVÁ OPRAVA: (1.0 - shadowFactor)
    vec3 color = ambient + (1.0 - shadowFactor) * Lo;

    // gamma korekce
    color = pow(color, vec3(1.0/2.2));

    out_color = vec4(color, 1.0);
}
