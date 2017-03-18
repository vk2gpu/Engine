RWTexture2D<float4> tex : register(u0);

[numthreads(8,8,1)]
void CTestTex(int3 id : SV_DispatchThreadID)
{
	float3 col = (float3(id.xy, 0.0) / 128.0);
	tex[id.xy] = float4(col.xyz, 1.0);
}
