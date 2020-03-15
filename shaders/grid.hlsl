struct VS_INPUT
{
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PS_INPUT
{
    linear float4 position : SV_POSITION;
    // linear float3 world : POSITION;
    linear float3 ray : RAY;
    linear float2 uv : TEXCOORD0;
};

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 b0View;
    float4x4 b0Projection;
    float3 b0LightDirection;
    float3 b0LightColor;
    float3 b0CameraPosition;
    float3 b0ScreenSizeFovY;
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

    ps.position = float4(vs.position, 0.999, 1);
    // ps.position = float4(vs.position, 0, 1);
    float halfFov = b0ScreenSizeFovY.z / 2;
    float t = tan(halfFov);
    float aspectRatio = b0ScreenSizeFovY.x / b0ScreenSizeFovY.y;
    ps.ray = mul(float4(vs.position.x * t * aspectRatio, vs.position.y * t, -1, 0), b0View).xyz;

    ps.uv = vs.uv;
    return ps;
}

// https://gamedev.stackexchange.com/questions/141916/antialiasing-shader-grid-lines
float grid(float2 uv, float2 speeds, float thickness)
{
    float2 wrapped = frac(uv + float2(0.5, 0.5)) - 0.5f;
    float2 range = abs(wrapped);

    // float2 speeds;
    /* // Euclidean norm gives slightly more even thickness on diagonals
                float4 deltas = float4(ddx(i.uv), ddy(i.uv));
                speeds = sqrt(float2(
                            dot(deltas.xz, deltas.xz),
                            dot(deltas.yw, deltas.yw)
                         ));
                */
    // Cheaper Manhattan norm in fwidth slightly exaggerates thickness of diagonals
    // speeds = fwidth(uv);

    float2 pixelRange = range / speeds;
    float lineWeight = saturate(min(pixelRange.x, pixelRange.y) - thickness);
    return 1 - lineWeight;
}

float gridX(float x)
{
    float wrapped = frac(x + 0.5) - 0.5;
    float range = abs(wrapped);
    return range;
}

float4 PSMain(PS_INPUT ps) : SV_Target
{
    float3 n = b0CameraPosition.y >= 0 ? float3(0, -1, 0) : float3(0, 1, 0);

    float d = dot(n, ps.ray);
    clip(d);

    float t = dot(-b0CameraPosition, n) / d;
    float3 world = b0CameraPosition + t * ps.ray;

    float3 forward = float3(b0View[2][0], b0View[2][1], b0View[2][2]);
    float fn = 0.2 + smoothstep(0, 0.8, abs(dot(forward, n)));
    float near = 30 * fn;
    float far = near * 5;

    float distance = length(b0CameraPosition - world);
    float fade = smoothstep(0.8, 0, (distance - near) / (far - near));

    float thickness = 0.01 / distance;
    float2 dd = fwidth(world.xz);
    float gx = gridX(world.x);
    float gy = gridX(world.z);
    float lx = saturate(gx - thickness) / dd.x;
    float ly = saturate(gy - thickness) / dd.y;
    float c = 1 - min(lx, ly);
    c *= fade;

    return float4(c, c, c, 1);

    // float w = grid(world.xz, fwidth(ps.uv), 2 / distance);

    // float value = fade * w;
    // float4 color = float4(0.8, 0.8, 0.8, 1.0);
    // return color * float4(value, value, value, 1);

    // return lerp(float4(c, c, c, 1), float4(0, 0, 0, 0), lineWeight);
}
