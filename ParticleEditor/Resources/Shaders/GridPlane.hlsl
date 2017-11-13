#include "include/Camera.hlsli"
#include "include/LightCalc.hlsli"

cbuffer cb0 : register(b0) { Camera camera; };
cbuffer cb1 : register(b1) { DirectionalLight globalLight; };

struct VSInput {
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VSOut {
    float4 position : SV_POSITION;
    float3 wposition : POSITION;
    float2 uv : TEXCOORD;
};

VSOut VS(VSInput input)
{
    VSOut output;
    output.uv = input.uv;
    output.wposition = input.position;

    output.position = mul(camera.viewProjection, float4(input.position, 1.0));

    return output;
}

Texture2D GridTexture : register(t0);
SamplerState GridSampler : register(s0);

float4 PS(VSOut input) : SV_TARGET
{
    float3 viewDir = normalize(camera.position - input.wposition);

    float3 lights = float3(0, 0, 0);
    lights += calcLight(globalLight, input.wposition, float3(0, 1, 0), viewDir, 0.1);
    lights += calcAllLights(input.position, input.wposition, float3(0, 1, 0), viewDir, 0.1);

    float3 grid = GridTexture.Sample(GridSampler, input.uv).rgb;

    return float4(grid * lights, 0.8f);
}