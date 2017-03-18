RWBuffer<int> buf : register(u0);

[numthreads(8,1,1)]
void CTestBuf(int3 id : SV_DispatchThreadID)
{
	buf[id.x] = id.x;
}
