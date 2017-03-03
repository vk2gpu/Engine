struct VOut
{
	float4 position : SV_POSITION;
};

VOut VShader(float4 position : POSITION)
{
	VOut output;
	output.position = position;
	return output;
}
