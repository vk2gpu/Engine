Texture2D<float4> tex : register(t0);
sampler smp : register(s0);

struct VOut
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
};

VOut VShader(float2 position : POSITION, float4 color : COLOR0, float2 uv : TEXCOORD0)
{
	VOut output;
	output.position = float4(position, 0.0, 1.0);
	output.color = color;
	output.uv = uv;
	return output;
}

float4 PShader(VOut vout) : SV_Target0
{
	return vout.color * tex.Sample(smp, vout.uv);
}
