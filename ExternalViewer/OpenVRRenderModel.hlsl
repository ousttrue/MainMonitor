R""(
cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 b0View;
    float4x4 b0Projection;
    float3 b0LightDir;
    float3 b0LightColor;
};
cbuffer NodeConstantBuffer : register(b1)
{
    float4x4 b1World;
};
cbuffer MaterialConstantBuffer: register(b2)
{
	float4 b2Diffuse;
	float3 b2Ambient;
	float3 b2Specular;
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

    result.position = mul(b0Projection, mul(b0View, mul(b1World, float4(position, 1))));
    result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.uv, 0, 1);
}
)""