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
    float4x4 b0View : CAMERA_VIEW;
    float4x4 b0Projection : CAMERA_PROJECTION;
    float3 b0LightDirection : LIGHT_DIRECTION;
    float3 b0LightColor : LIGHT_COLOR;
    float3 b0CameraPosition : CAMERA_POSITION;
    float fovY : CAMERA_FOVY;
    float3 b0ScreenSize : RENDERTARGET_SIZE;
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

    ps.position = float4(vs.position, 1, 1);
    // ps.position = float4(vs.position, 0, 1);
    float halfFov = b0ScreenSizeFovY.z / 2;
    float t = tan(halfFov);
    float aspectRatio = b0ScreenSizeFovY.x / b0ScreenSizeFovY.y;
    ps.ray = mul(float4(vs.position.x * t * aspectRatio, vs.position.y * t, -1, 0), b0View).xyz;

    ps.uv = vs.uv;
    return ps;
}

// https://gamedev.stackexchange.com/questions/141916/antialiasing-shader-grid-lines
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
    float far = near * 3;

    float distance = length(b0CameraPosition - world);
    float fade = smoothstep(1, 0, (distance - near) / (far - near));

    int modX = trunc(abs(world.x) + 0.5) % 5;
    int modY = trunc(abs(world.z) + 0.5) % 5;
    float thicknessX = modX ? 0.005 : 0.02;
    float thicknessY = modY ? 0.005 : 0.02;

    float2 dd = fwidth(world.xz);
    float gx = gridX(world.x);
    float gy = gridX(world.z);
    float lx = 1 - saturate(gx - thicknessX) / dd.x;
    float ly = 1 - saturate(gy - thicknessY) / dd.y;
    float c = max(lx, ly);
    c *= fade;

    return float4(float3(0.8, 0.8, 0.8) * c, 1);
}
