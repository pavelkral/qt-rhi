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
    float debugMode;
    float lightIntensity;
} ubo;

float shadowCalculation() {
    // Převod pozice fragmentu do prostoru světla
    vec4 fragPosLightSpace = ubo.lightSpace * vec4(frag_pos, 1.0);

    // Homogenní souřadnice na NDC (Normalizované Souřadnice Zařízení)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Převod z NDC na UV souřadnice (0.0 až 1.0)
    vec2 uv_shadow = projCoords.xy * 0.5 + 0.5;

    // Získání hloubky uložené v mapě stínů
    float closestDepth = texture(tex_shadows, uv_shadow).r;

    // Aktuální hloubka fragmentu v prostoru světla
    float currentDepth = projCoords.z;
//float currentDepth = projCoords.z * 0.5 + 0.5;
    // Biasing - přidáme malou hodnotu, abychom zabránili "acne" efektu
    // (fragmenty, které by měly být osvětlené, jsou omylem ve stínu)
    float bias = max(0.0005 * (1.0 - dot(normalize(frag_normal), normalize(ubo.lightPos.xyz - frag_pos))), 0.0005);

    // Percentage-Closer Filtering (PCF) pro plynulejší stíny
    float shadowFactor = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(tex_shadows, 0));
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(tex_shadows, uv_shadow + vec2(x, y) * texelSize).r;
            shadowFactor += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadowFactor /= 9.0;

    // Pokud je fragment mimo rozsah mapy stínů (např. za světlem),
    // je považován za plně osvětlený.
    if(uv_shadow.x > 1.0 || uv_shadow.x < 0.0 || uv_shadow.y > 1.0 || uv_shadow.y < 0.0)
        shadowFactor = 1.0;

    return shadowFactor;
}

void main() {
    // --- TBN ---
    vec3 T = normalize(frag_tangent);
    vec3 B = normalize(frag_bitangent);
    vec3 N = normalize(frag_normal);
    mat3 TBN = mat3(T, B, N);

    // --- směry ---
    vec3 V = normalize(ubo.camPos.xyz - frag_pos);
    vec3 L = normalize(ubo.lightPos.xyz - frag_pos);

    // --- textury ---
    // Poznámka: Pokud jsou albedo a PBR textury sRGB, je tento krok nutný.
    // Vaše PBR textury by měly být načtené do lineárního prostoru.
    vec3 albedo = texture(tex_albedo, frag_uv).rgb;
    float metallic  = texture(tex_metallic, frag_uv).r;
    float roughness = texture(tex_roughness, frag_uv).r;
    float ao      = texture(tex_ao, frag_uv).r;
    float height    = texture(tex_height, frag_uv).r;

    vec3 tangentNormal = texture(tex_normal, frag_uv).xyz * 2.0 - 1.0;
    vec3 N_world = normalize(TBN * tangentNormal);

    // --- debug režimy ---
    // if (ubo.debugMode == 0.0) {
    //     out_color = vec4(albedo, 1.0);
    //     return;
    // } else if (ubo.debugMode == 1.0) {
    //     out_color = vec4(N_world * 0.5 + 0.5, 1.0);
    //     return;
    // } else if (ubo.debugMode == 2.0) {
    //     out_color = vec4(vec3(metallic), 1.0);
    //     return;
    // } else if (ubo.debugMode == 3.0) {
    //     out_color = vec4(vec3(roughness), 1.0);
    //     return;
    // } else if (ubo.debugMode == 4.0) {
    //     out_color = vec4(vec3(ao), 1.0);
    //     return;
    // } else if (ubo.debugMode == 5.0) {
    //     out_color = vec4(vec3(height), 1.0);
    //     return;
    // }

    // --- PBR ---
    vec3 H = normalize(V + L);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

    float NdotL = max(dot(N_world, L), 0.0);
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 radiance = ubo.color.rgb * ubo.lightIntensity;

    vec3 diffuse = kD * albedo / 3.141592;
    vec3 specular = kS;

    vec3 Lo = (diffuse + specular) * radiance * NdotL;
    vec3 ambient = albedo * ao * 0.1;

    // --- Výpočet stínů a jejich aplikace ---
    float shadowFactor = shadowCalculation();

    // Stín ovlivňuje pouze přímé osvětlení 'Lo', nikoliv ambientní
    vec3 color = ambient + shadowFactor * Lo;

    // --- gamma korekce ---
    // Převod z lineárního prostoru do sRGB pro správné zobrazení na monitoru
    // Standardní exponent je 1.0/2.2. Váš předchozí kód měl 1.0/2.0.
    color = pow(color, vec3(1.0/2.2));

    out_color = vec4(color, 1.0);
}
