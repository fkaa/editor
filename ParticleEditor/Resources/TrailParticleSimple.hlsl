cbuffer Camera : register(b0)
{
	float4x4 World;
	float4x4 View;
	float4x4 Proj;
	float4x4 NormalMatrix;
	float4x4 InverseWVP;
};

struct VSIn {
	float3 position : POSITION;
	float2 size : SIZE;
};

VSIn VS(VSIn input) {
	return input;
}

struct GSOut
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

#define TRAIL_COUNT 16

[maxvertexcount(4)]
void GS(lineadj VSIn inp[4], inout TriangleStream<GSOut> output)
{
	GSOut v0, v1, v2, v3;

	float3 p0, p1, p2, p3;
	p0 = inp[0].position; p1 = inp[1].position;
	p2 = inp[2].position; p3 = inp[3].position;
	float3 n0 = normalize(p1 - p0);
	float3 n1 = normalize(p2 - p1);
	float3 n2 = normalize(p3 - p2);
	float3 u = normalize(n0 + n1);
	float3 v = normalize(n1 + n2);

	//This can be varied to customize graftals
	float rad0 = inp[1].size.x;
	float rad1 = inp[2].size.x;

	//Calculate the ribs
	float3 orient = -float3(View._m20, View._m21, View._m22);
	float3 r0 = cross(u, orient) * rad0;
	float3 r1 = cross(v, orient) * rad1;

	float4x4 VP = mul(Proj, View);

	v0.pos = mul(VP, float4(p1 - r0, 1));
	v1.pos = mul(VP, float4(p1 + r0, 1));
	v2.pos = mul(VP, float4(p2 - r1, 1));
	v3.pos = mul(VP, float4(p2 + r1, 1));

	v0.uv = float2(1, 0);
	v1.uv = float2(1, 0);
	v2.uv = float2(1, 0);
	v3.uv = float2(1, 0);

	output.Append(v0);
	output.Append(v1);
	output.Append(v2);
	output.Append(v3);
}

float4 PS(GSOut input) : SV_Target0
{
	return float4(1, 1, 1, 1);
}