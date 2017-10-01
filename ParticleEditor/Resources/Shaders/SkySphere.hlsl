cbuffer Camera : register(b0)
{
	float4x4 View;
	float4x4 Proj;
	float4 Pos;
}

struct VSInput {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct VSOut {
	float4 position : SV_POSITION;
	float3 wposition : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

VSOut VS(VSInput input)
{
	VSOut output;
	output.normal = input.normal;
	output.uv = input.uv;
	output.wposition = input.position;

	output.position = mul(Proj, mul(View, float4(input.position*15.f, 1.0)));
	output.position.w = output.position.z;

	return output;
}

float4 PS(VSOut input) : SV_TARGET
{
	float3 col = lerp(float3(0.078, 0.180, 0.329), float3(0.459, 0.541, 0.659), saturate(input.wposition.y*0.5+0.75));

	return float4(col, 1.0f);
}