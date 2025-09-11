
#pragma qt_vertex_shader
#pragma qt_resource_name "vertexShader"

// Constant buffer pro uniformní proměnné.
// V HLSL odpovídá GLSL 'uniform Ubo'.
// 'register(b0)' mapuje na binding slot 0.
cbuffer Ubo : register(b0)
{
    // HLSL používá standardně row-major uspořádání matic,
    // na rozdíl od GLSL (column-major). Pořadí násobení je tedy
    // vektor * matice.
    float4x4 mvp;
    float4 opacity;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 lightSpace;
    float4 lightPos;
    float4 color;
};

// Struktura pro data posílaná z Vertex Shaderu do Pixel Shaderu.
// Nahrazuje GLSL 'in'/'out' proměnné mezi shadery.
// Každý prvek má "sémantiku", která určuje, jak se s ním má zacházet.
struct PS_INPUT
{
    float4 position   : SV_Position; // Výstupní pozice (clip space), sémantika je povinná
    float2 uv         : TEXCOORD0;   // UV souřadnice
    float3 normal     : NORMAL;      // Normála v world space
    float3 fragPos    : TEXCOORD1;   // Pozice fragmentu v world space
};

//=================================================================================
// Vertex Shader
//=================================================================================

// Vstupní parametry vertex shaderu jsou definovány sémantikou,
// která odpovídá layoutu vertex bufferu.
PS_INPUT mainVS(
    float3 in_pos    : POSITION,
    float3 in_normal : NORMAL,
    float2 in_uv     : TEXCOORD0)
{
    PS_INPUT output;

    // Transformace pozice do world space (vektor * matice)
    float4 worldPos = mul(float4(in_pos, 1.0f), model);

    // Transformace do clip space
    // Pořadí: model -> view -> projection
    float4x4 viewProj = mul(view, projection);
    output.position = mul(worldPos, viewProj);

    // Předání UV souřadnic a pozice ve world space do pixel shaderu
    output.uv = in_uv;
    output.fragPos = worldPos.xyz;

    // Správná transformace normály (pro row-major matice)
    // n' = normalize(mul(n, (float3x3)inverse(transpose(model))))
    float3x3 normalMatrix = (float3x3)inverse(transpose(model));
    output.normal = mul(in_normal, normalMatrix);

    return output;
}
