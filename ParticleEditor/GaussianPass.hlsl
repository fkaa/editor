struct VSIn {
	float2 Pos : POSITION;
	float2 Uv : TEXCOORD;
};

struct VSOut {
	float4 Pos : SV_POSITION;
	float2 Uv : TEXCOORD;
};

VSOut VS(VSIn input)
{
	VSOut output;

	output.Pos = float4(input.Pos, 0.0, 1.0);
	output.Uv = input.Uv;

	return output;
}


Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 PS(VSOut input) : SV_Target
{
	return Texture.Sample(Sampler, input.Uv);
}

static const float weights[5] = {
	0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
};

float4 GaussianX(VSOut input) : SV_Target
{
	float3 result = Texture.Sample(Sampler, input.Uv).rgb * weights[0];

	for (int i = 1; i < 5; ++i)
	{
		result += Texture.Sample(Sampler, input.Uv, int2(i, 0)) * weights[i];
		result += Texture.Sample(Sampler, input.Uv, int2(-i, 0)) * weights[i];
	}

	return float4(result, 1.0);
}

float4 GaussianY(VSOut input) : SV_Target
{
	float3 result = Texture.Sample(Sampler, input.Uv).rgb * weights[0];

	for (int i = 1; i < 5; ++i)
	{
		result += Texture.Sample(Sampler, input.Uv, int2(0, i)) * weights[i];
		result += Texture.Sample(Sampler, input.Uv, int2(0, -i)) * weights[i];
	}

	return float4(result, 1.0);
}