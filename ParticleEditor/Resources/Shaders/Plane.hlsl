cbuffer Camera : register(b0) {
	float4x4 World;
	float4x4 View;
	float4x4 Proj;
};

cbuffer ShadowCamera : register(b5) {
	float4x4 ShadowWorld;
	float4x4 ShadowView;
	float4x4 ShadowProj;
}

cbuffer dirLigth : register(b2)
{
	float3 dLightDirection;
	float4 dLightcolor;
}

SamplerState ShadowSampler : register(s1);
Texture2D ShadowMap : register(t1);

float GetShadow(float3 normal, float3 lightDir, float4 coords)
{
	float3 proj = coords.xyz;
	proj = proj * 0.5 + 0.5;
	proj.y = 1.0 - proj.y;

	float bias = max(0.0005 * (1.0 - dot(normal, normalize(lightDir))), 0.0005);
	float shadow = 0;

	[unroll]
	for (int x = -1; x <= 1; ++x) {
		[unroll]
		for (int y = -1; y <= 1; ++y) {
			float shadowDepth = ShadowMap.Sample(ShadowSampler, proj.xy, int2(x, y)).r;
			if (shadowDepth - bias < proj.z)
				shadow += 1;
		}
	}

	return 1 - (shadow / 9);
}

struct VSOut {
	float3 WorldPos : POSITION;
	float4 Pos : SV_POSITION;
};

VSOut VS(float3 pos : POSITION)
{
	VSOut output;

	output.WorldPos = pos;
	output.Pos = mul(Proj, mul(View, float4(pos, 1.0)));

	return output;
}

SamplerState PlaneSampler : register(s0);
Texture2D PlaneTexture : register(t0);

float4 PS(VSOut input) : SV_TARGET
{
	float4 coords = mul(ShadowProj, mul(ShadowView, mul(ShadowWorld, input.WorldPos)));
	//float shadow = GetShadow(float3(0, 1, 0), dLightDirection, coords);

	float3 plane = PlaneTexture.Sample(PlaneSampler, input.WorldPos.xz / 4.f).rgb;

	return float4(plane, 1.0);
}