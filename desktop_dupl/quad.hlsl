R""(
Texture2D tex0;
SamplerState samplerState;

struct VS_IN
{
    float2 position: POSITION;
    float2 uv: TEXCOORD0;
};

struct VS_OUT
{
    float4 position: SV_POSITION;
    float2 uv: TEXCOORD0;
};

typedef VS_OUT PS_IN;

VS_OUT vsMain(VS_IN input)
{
    VS_OUT Output;
	Output.position = float4(input.position.x, input.position.y, 0, 1);
    Output.uv = input.uv;
    return Output;    
}

float4 psMain(PS_IN input) : SV_TARGET
{
	return tex0.Sample(samplerState, input.uv);
}
)""
