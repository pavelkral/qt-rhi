
#pragma qt_vertex_shader
#pragma qt_resource_name "vertexShader"

cbuffer Ubo : register(b0)
{
    // HLSL  row-major
    // GLSL (column-major).
    // vector * matrix.
    float4x4 mvp;
    float4 opacity;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 lightSpace;
    float4 lightPos;
    float4 color;
};

struct PS_INPUT
{
    float4 position   : SV_Position;
    float2 uv         : TEXCOORD0;
    float3 normal     : NORMAL;
    float3 fragPos    : TEXCOORD1;
};

//=================================================================================
// Vertex Shader
//=================================================================================

PS_INPUT mainVS(
    float3 in_pos    : POSITION,
    float3 in_normal : NORMAL,
    float2 in_uv     : TEXCOORD0)
{
    PS_INPUT output;

    float4 worldPos = mul(float4(in_pos, 1.0f), model);

    float4x4 viewProj = mul(view, projection);
    output.position = mul(worldPos, viewProj);

    output.uv = in_uv;
    output.fragPos = worldPos.xyz;

    float3x3 normalMatrix = (float3x3)inverse(transpose(model));
    output.normal = mul(in_normal, normalMatrix);

    return output;
}
