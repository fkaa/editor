cbuffer Camera : register(b0)
{
	float4x4 View;
	float4x4 Proj;
};

struct VSIn {
	// vb(0)
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;

	// ib(1)
	float4x4 model : MODEL;
	float age : AGE;
	int idx : IDX;
};

struct VSOutput {
	float4 position : SV_POSITION;
	float3 worldPos : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	float age : AGE;
};

Texture2D Noise : register(t3);
SamplerState Sampler : register(s1);

VSOutput VS(VSIn input) {
	VSOutput output;

	float3 expand = Noise.SampleLevel(Sampler, input.uv + float2(0, input.age*0.01), 0).r * input.normal;

	output.position = mul(Proj, mul(View, float4(input.position+expand*0.01, 1)));
	output.worldPos = input.position;
	output.normal = input.normal;
	output.uv = input.uv;
	output.age = input.age;

	return output;
}

float4 PS(VSOutput input) : SV_Target0
{
	float noise = Noise.SampleLevel(Sampler, input.uv + float2(0, input.age*0.1), 0).r;

	clip(saturate(step(0.25, noise)) - 0.5);

	return float4(float3(0.12, 0.4, 0.9) * noise, 1);
}
