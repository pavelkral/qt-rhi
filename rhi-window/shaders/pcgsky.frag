#version 450

layout(location = 0) in vec2 v_clipSpace;
layout(location = 0) out vec4 FragColor;

layout(std140, binding = 0) uniform Ubo {
    mat4 invProjection;
    mat4 invView;
    vec4 sunDir;
    vec4 params;
} u_ubo;

// --- ŠUMOVÉ FUNKCE PRO MRAKY (FBM - Fractal Brownian Motion) ---

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 st) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 6; i++) {
        value += amplitude * noise(st * frequency);
        st *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}
// ---------------------------------------------------------------


void main() {
    // Transformace pro směr pohledu
    vec4 clip = vec4(v_clipSpace, 1.0, 1.0);

    vec4 view = u_ubo.invProjection * clip;
    view = view / view.w;
    vec4 world = u_ubo.invView * vec4(view.xyz, 0.0);
    vec3 direction = normalize(world.xyz);

    // --- 1. Základní barva oblohy a slunce ---
    // ZMĚNA: Přístup k u_sunDirection (použije se jen .xyz)
    vec3 sunDir = normalize(u_ubo.sunDir.xyz);

    float sunHeight = smoothstep(-0.1, 0.2, sunDir.y);
    vec3 dayTopColor = vec3(0.5, 0.7, 1.0);
    vec3 sunsetTopColor = vec3(0.3, 0.4, 0.6);
    vec3 dayBottomColor = vec3(0.9, 0.9, 1.0);
    vec3 sunsetBottomColor = vec3(0.9, 0.6, 0.4);
    vec3 topColor = mix(sunsetTopColor, dayTopColor, sunHeight);
    vec3 bottomColor = mix(sunsetBottomColor, dayBottomColor, sunHeight);
    float t = 0.5 * (direction.y + 1.0);
    vec3 skyColor = mix(bottomColor, topColor, t);
    vec3 sunColor = vec3(1.0, 0.9, 0.8);
    float dotSun = dot(direction, sunDir);
    float sunGlow = smoothstep(0.998, 1.0, dotSun);
    float sunDisk = smoothstep(0.9999, 1.0, dotSun);
    vec3 finalColor = skyColor + sunColor * sunGlow * 0.5 + sunColor * sunDisk;

    // -------------------------------------------------------------------
    // --- 2. Logika pro Procedurální Mraky ---
    // -------------------------------------------------------------------
    float azimuth = atan(direction.x, direction.z) * 0.15915 + 0.5;
    vec2 cloudUV = vec2(azimuth, direction.y);
    float cloudScale = 5.0;
    cloudUV.x *= cloudScale;
    cloudUV.y *= cloudScale * 2.0;
    cloudUV.x += u_ubo.params.x * 0.005;
    cloudUV.y += u_ubo.params.x * 0.002;

    float density = fbm(cloudUV);
    float cloudAltitude = 0.0;
    float cloudThickness = 0.5;
    float heightMask = 1.0 - abs(direction.y - cloudAltitude) / cloudThickness;
    heightMask = clamp(heightMask, 0.0, 1.0);
    heightMask = pow(heightMask, 2.0);
    float finalDensity = pow(density, 2.0) * heightMask;
    float cloudThreshold = 0.3;
    float cloudMask = smoothstep(cloudThreshold, cloudThreshold + 0.2, finalDensity);
    float lightDot = dot(direction, sunDir);
    float mieScatter = pow(smoothstep(-0.3, 1.0, lightDot), 4.0);
    vec3 cloudColor = mix(vec3(0.9), topColor * 1.2, sunHeight);
    vec3 illuminatedColor = cloudColor * (1.0 + mieScatter * 1.5);
    vec3 shadowColor = vec3(0.2, 0.3, 0.4) * (1.0 - sunHeight * 0.5);
    float darkness = pow(clamp(-direction.y, 0.0, 1.0), 3.0);
    vec3 finalCloudColor = mix(illuminatedColor, shadowColor, darkness * 0.5 + (1.0 - lightDot) * 0.2);

    finalColor = mix(finalColor, finalCloudColor, cloudMask);
    FragColor = vec4(finalColor, 1.0);
}
