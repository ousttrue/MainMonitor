R""(
cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 view;
    float4x4 projection;
};
cbuffer SceneConstantBuffer : register(b1)
{
    float4x4 world;
};

struct VSInput
{
    float3 position : SV_POSITION;
    float3 normal: NORMAL;
    float2 uv : TEXCOORD0;
};
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD0)
{
    PSInput result;

    result.position = mul(projection, mul(view, mul(world, float4(position, 1))));
    result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.uv, 0, 1);
}
)""