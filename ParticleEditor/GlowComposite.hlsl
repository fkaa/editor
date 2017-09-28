struct VSOut {
	float4 Pos : SV_POSITION;
	float2 Uv : TEXCOORD;
};

Texture2D Source : register(t0);
Texture2D Glow : register(t1);

SamplerState Sampler : register(s0);

float4 PS(VSOut input) : SV_Target
{
	float4 src = Source.Sample(Sampler, input.Uv);

	float4 glow = 0;
	[unroll]
	for (int i = 1; i < 11; ++i) {
		glow += Glow.SampleLevel(Sampler, input.Uv, i);
	}
	
	return src + glow / 10.f;
}