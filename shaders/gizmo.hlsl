SamplerState s0 : register(s0);
Texture2D t0 : register(t0);
cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 b0View;
    float4x4 b0Projection;
    float3 b0LightDirection;
    float3 b0LightColor;
    float3 uEye;
};

cbuffer NodeConstantBuffer : register(b1)
{
    float4x4 b1World : WORLD;
};
// cbuffer MaterialConstantBuffer: register(b2)
// {
// 	float4 b2Diffuse;
// 	float3 b2Ambient;
// 	float3 b2Specular;
// };

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR0;
};
struct PS_INPUT
{
    linear float4 position : SV_POSITION;
    linear float3 normal : NORMAL;
    linear float4 color : COLOR0;
    linear float3 world : POSITION;
};

PS_INPUT VSMain(VS_INPUT vs)
{
    PS_INPUT ps;
    ps.world = float4(vs.position, 1).xyz;
    ps.position = mul(mul(b0Projection, b0View), float4(ps.world, 1));
    ps.normal = vs.normal;
    ps.color = vs.color;
    return ps;
}

float4 PSMain(PS_INPUT ps) : SV_Target
{
    float3 light = float3(1, 1, 1) * max(dot(ps.normal, normalize(uEye - ps.world)), 0.50) + 0.25;
    return ps.color * float4(light, 1);
}

technique MainTec0
{
    pass DrawObject
    {
        VertexShader = compile vs_3_0 VSMain();
        PixelShader = compile ps_3_0 PSMain();
    }
}
