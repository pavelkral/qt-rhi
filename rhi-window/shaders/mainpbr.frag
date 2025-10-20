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
    vec4 misc1; // misc1.w bude náš přepínač (1, 2, nebo 3)
} ubo;


// --- PBR pomocné funkce (beze změny) ---
const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom    = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom    = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// --- Spojená Shadow map funkce ---
float shadowCalculation(vec3 normal, vec3 fragPos)
{
    vec4 fragPosLightSpace = ubo.lightSpace * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    int mode = int(ubo.misc1.w);

    // --- ROZDÍL 1 & 2: Transformace souřadnic a kontrola hloubky ---
    if (mode == 1) {
        // Shader 1: (nesprávná) Vulkan v1 transformace
        projCoords = projCoords * 0.5 + 0.5;
        if (projCoords.z > 1.0) {
            return 0.0;
        }
    } else if (mode == 2) {
        // Shader 2: D3D/OpenGL Y-flip transformace
        projCoords.x = projCoords.x * 0.5 + 0.5;
        projCoords.y = projCoords.y * -0.5 + 0.5; // Převrácení Y
        if (projCoords.z > 1.0 || projCoords.z < 0.0) {
            return 0.0;
        }
    } else { // mode == 3
        // Shader 3: (správná) Vulkan v2 transformace
        projCoords.xy = projCoords.xy * 0.5 + 0.5;
        if (projCoords.z > 1.0 || projCoords.z < 0.0) {
            return 0.0;
        }
    }

    float currentDepth = projCoords.z;

    // --- Společná část: Bias ---
    float bias = max(0.005 * (1.0 - dot(normal, normalize(ubo.lightPos.xyz - fragPos))), 0.001);

    // --- Společná část: PCF smyčka ---
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(tex_shadows, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(tex_shadows, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}


void main()
{
    // --- TBN matice (společné) ---
    vec3 T = normalize(frag_tangent);
    vec3 B = normalize(frag_bitangent);
    vec3 N = normalize(frag_normal);
    mat3 TBN = mat3(T, B, N);

    int mode = int(ubo.misc1.w);
    vec3 N_world;
    float roughness;

    // --- ROZDÍL 3 & 4: Normály a Roughness ---
    if (mode == 3) {
        // Shader 3: Vylepšené normály a omezená roughness
        vec3 N_local = texture(tex_normal, frag_uv).xyz * 2.0 - 1.0;

        // Pozn: Tuto hodnotu můžete také poslat v UBO, např. ubo.misc.x
        float normalStrength = 2.0;

        N_local.xy *= normalStrength;
        N_local = normalize(N_local);
        N_world = normalize(TBN * N_local);

        roughness = max(texture(tex_roughness, frag_uv).r, 0.04);

    } else {
        // Shader 1 & 2: Standardní výpočty
        N_world = normalize(TBN * (texture(tex_normal, frag_uv).xyz * 2.0 - 1.0));
        roughness = texture(tex_roughness, frag_uv).r;
    }

    // --- Stíny (volá spojenou funkci) ---
    float shadowFactor = shadowCalculation(N_world, frag_pos);

    // --- Směry (společné) ---
    vec3 V = normalize(ubo.camPos.xyz - frag_pos);
    vec3 L = normalize(ubo.lightPos.xyz - frag_pos);
    vec3 H = normalize(V + L);

    // --- Textury (společné) ---
    vec3 albedo = texture(tex_albedo, frag_uv).rgb;
    float metallic = texture(tex_metallic, frag_uv).r;
    float ao = texture(tex_ao, frag_uv).r;

    // --- PBR výpočty (společné, ale závislé na N_world a roughness) ---
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

    // --- Finální osvětlení (společné) ---
    vec3 radiance = ubo.color.rgb * ubo.misc1.y;
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    vec3 ambient = albedo * ao * 0.1;
    vec3 color = ambient + (1.0 - shadowFactor) * Lo;

    // Gamma korekce
    color = pow(color, vec3(1.0/2.2));

    out_color = vec4(color, 1.0);
}

