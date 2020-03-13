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
    float3 normal: NORMAL;
    float4 color : COLOR0;
};
struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal: NORMAL;
    float4 color : COLOR0;
};

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float4 color : COLOR0)
{
    PSInput result;

    result.position = mul(b0Projection, mul(b0View, float4(position, 1)));
    result.normal = normal;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}

technique MainTec0 {
    pass DrawObject {
        VertexShader = compile vs_3_0 VSMain();
        PixelShader  = compile ps_3_0 PSMain();
    }
}
