#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_fragpos;
layout(location = 3) in mat3 in_tbn; // tangent, bitangent, normal matrix pro normal mapping

layout(location = 0) out vec4 out_color;

// Textury
layout(binding = 0) uniform sampler2D tex_albedo;
layout(binding = 1) uniform sampler2D tex_normal;
layout(binding = 2) uniform sampler2D tex_metallic;
layout(binding = 3) uniform sampler2D tex_roughness;
layout(binding = 4) uniform sampler2D tex_ao;
layout(binding = 5) uniform sampler2D tex_height;

layout(std140, binding = 0) uniform Ubo {
    mat4 mvp;
    vec4 opacity;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpace;
    vec4 lightPos;
    vec4 camPos;
    vec4 color;
} ubo;


// ------------------------------------------------------
// Pomocné funkce pro PBR
// ------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (3.141592 * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Parallax mapping (jednoduchý, volitelný)
vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    float height = texture(tex_height, texCoords).r;
    float heightScale = 0.05;
    vec2 p = viewDir.xy / viewDir.z * (height * heightScale);
    return texCoords - p;
}


void main() {
    // Camera + světlo
    vec3 V = normalize(ubo.camPos.xyz - in_fragpos);
    vec3 L = normalize(ubo.lightPos.xyz - in_fragpos);
    vec3 H = normalize(V + L);

    // Parallax mapování (přepíše UV)
    vec2 uv = parallaxMapping(in_uv, V);

    // Načtení textur
    vec3 albedo     = pow(texture(tex_albedo, uv).rgb, vec3(2.2)); // sRGB->linear
    float metallic  = texture(tex_metallic, uv).r;
    float roughness = texture(tex_roughness, uv).r;
    float ao        = texture(tex_ao, uv).r;

    // Normál z normal mapy
    vec3 tangentNormal = texture(tex_normal, uv).xyz * 2.0 - 1.0;
    vec3 N = normalize(in_tbn * tangentNormal);

    // Cook-Torrance BRDF
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denom       = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular     = numerator / denom;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / 3.141592 + specular) * NdotL;

    // Ambient (s AO)
    vec3 ambient = ao * albedo * 0.03;

    vec3 color = ambient + Lo;

    // Gamma korekce
    color = pow(color, vec3(1.0/2.2));

    out_color = vec4(color, 1.0);
}

