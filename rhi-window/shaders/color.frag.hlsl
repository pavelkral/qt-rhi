
#pragma qt_fragment_shader
#pragma qt_resource_name "fragmentShader"

// --- Inputs from the vertex shader
struct FS_INPUT
{
    float2 in_uv      : TEXCOORD0;
    float3 in_normal  : TEXCOORD1;
    float3 in_fragpos : TEXCOORD2;
};

// --- Uniform Buffer (matches binding from the vertex shader)
cbuffer Ubo : register(b0)
{
    float4x4 mvp;
    float4 opacity;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 lightSpace;
    float4 lightPos;
    float4 color;
};

// --- Texture and Sampler State
Texture2D sampler_tex : register(t1);
SamplerState sampler_tex_sampler : register(s1);

// --- Main fragment shader function
float4 main(FS_INPUT input) : SV_TARGET
{
    float3 norm = normalize(input.in_normal);
    float3 lightDir = normalize(ubo.lightPos.xyz - input.in_fragpos);

    // Ambient
    float3 ambient = 0.3 * ubo.color.rgb;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * ubo.color.rgb;

    // Specular
    float3 viewDir = normalize(-input.in_fragpos); // camera is at (0,0,0) in view space
    float3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    float3 specular = 0.3 * spec * float3(1.0, 1.0, 1.0);

    float4 texColor = sampler_tex.Sample(sampler_tex_sampler, input.in_uv);
    float3 lighting = (ambient + diffuse + specular) * texColor.rgb;
    float alpha = texColor.a * ubo.opacity.x;

    return float4(lighting, alpha);
}
