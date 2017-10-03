cbuffer Camera : register(b0)
{
	float4x4 View;
	float4x4 Proj;
};

struct VSIn {
	float3 pos : POSITION;
	float2 scale : SIZE;
	float age : AGE;

	int idx : IDX;
};

struct GSOut {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float age : AGE;
};

VSIn VS(VSIn input)
{
	return input;
}

static const float4 UV = float4(0, 0, 1, 1);

[maxvertexcount(4)]
void GS(point VSIn inp[1], inout TriangleStream<GSOut> outstream)
{
	VSIn input = inp[0];

	GSOut output;
	output.age = input.age;

	float w = input.scale.x;
	float h = input.scale.y;
	/*float rot = input.rotation;

	float4x4 rotate = {
		cos(rot), -sin(rot), 0.0, 0.0,
		sin(rot), cos(rot),  0.0, 0.0,
		0.0,        0.0,     1.0, 0.0,
		0.0,        0.0,     0.0, 1.0
	};

	float4 N = mul(rotate, float4(0, w, 0, 0));
	float4 S = mul(rotate, float4(0, -w, 0, 0));
	float4 E = mul(rotate, float4(-h, 0, 0, 0));
	float4 W = mul(rotate, float4(h, 0, 0, 0));*/

	float4 N = float4(0, w, 0, 0);
	float4 S = float4(0, -w, 0, 0);
	float4 E = float4(-h, 0, 0, 0);
	float4 W = float4(h, 0, 0, 0);

	float4 pos = mul(View, float4(input.pos.xyz, 1.0));

	output.pos = mul(Proj, pos + N + W);
	output.uv = UV.xy;
	outstream.Append(output);

	output.pos = mul(Proj, pos + S + W);
	output.uv = UV.xw;
	outstream.Append(output);

	output.pos = mul(Proj, pos + N + E);
	output.uv = UV.zy;
	outstream.Append(output);

	output.pos = mul(Proj, pos + S + E);
	output.uv = UV.zw;
	outstream.Append(output);

}

Texture2D Plane : register(t0);
Texture2D Noise : register(t1);
SamplerState Sampler : register(s1);

float4 PS(GSOut input) : SV_Target0
{
	float3 noiser = Noise.Sample(Sampler, input.uv + float2(0, input.age*0.1)).r;
	float3 noiseg = Noise.Sample(Sampler, input.uv + float2(0, input.age*0.2)).g;
	float3 noiseb = Noise.Sample(Sampler, input.uv + float2(0, input.age*0.4)).b;
	float3 col = Plane.Sample(Sampler, input.uv + float2(sin(input.age)*0.1, input.age*0.4)).rgb;

	float3 final = col;
	float alpha = noiser * noiseg * noiseb;
	if (alpha > 0.5) alpha = 1;
	else alpha = 0;

	return float4(col, alpha);
}