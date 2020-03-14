struct VS_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORDS0;
};

struct PS_INPUT
{
    linear float4 position : SV_POSITION;
    linear float2 uv : TEXCOORDS0;
};

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

PS_INPUT VSMain(VS_INPUT vs)
{
    PS_INPUT ps;
    ps.position = float4(vs.position, 1);
    ps.uv = vs.uv;
    return ps;
}

float4 PSMain(PS_INPUT ps) : SV_Target
{
    return float4(ps.uv, 0, 1);
}
