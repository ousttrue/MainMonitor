SamplerState s0 : register(s0);
Texture2D t0 : register(t0);
cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 b0View;
    float4x4 b0Projection;
    float3 b0LightDirection;
    float3 b0LightColor;
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

struct VSInput
{
    float3 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};
struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

PSInput VSMain(VSInput vs)
{
    PSInput result;

    result.position = mul(b0Projection, mul(b0View, mul(b1World, float4(vs.position, 1))));
    result.normal = normalize(mul(b1World, float4(vs.normal, 0)).xyz);
    result.uv = vs.uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // float3 P = input.position.xyz;
    float4 vColor = t0.Sample(s0, input.uv);
    clip(vColor.w - 0.6);
    return vColor;
}
