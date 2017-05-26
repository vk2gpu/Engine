struct Mat44
{
	float m[16];
};

static inline uniform float dot(uniform float<3> a, uniform float<3> b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

static inline uniform float dot(uniform float<4> a, uniform float<4> b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

export void Mat44_MulArray(uniform int num, uniform Mat44 out[], uniform const Mat44 a[], uniform const Mat44 b[])
{
	for(uniform int i = 0; i < num; ++i)
	{
		uniform float<4> aRow0 = { a[i].m[0x0], a[i].m[0x1], a[i].m[0x2], a[i].m[0x3] };
		uniform float<4> aRow1 = { a[i].m[0x4], a[i].m[0x5], a[i].m[0x6], a[i].m[0x7] };
		uniform float<4> aRow2 = { a[i].m[0x8], a[i].m[0x9], a[i].m[0xa], a[i].m[0xb] };
		uniform float<4> aRow3 = { a[i].m[0xc], a[i].m[0xd], a[i].m[0xe], a[i].m[0xf] };

		uniform float<4> bCol0 = { b[i].m[0x0], b[i].m[0x4], b[i].m[0x8], b[i].m[0xc] };
		uniform float<4> bCol1 = { b[i].m[0x1], b[i].m[0x5], b[i].m[0x9], b[i].m[0xd] };
		uniform float<4> bCol2 = { b[i].m[0x2], b[i].m[0x6], b[i].m[0xa], b[i].m[0xe] };
		uniform float<4> bCol3 = { b[i].m[0x3], b[i].m[0x7], b[i].m[0xb], b[i].m[0xf] };

		out[i].m[0x0] = dot(aRow0, bCol0);
		out[i].m[0x1] = dot(aRow0, bCol1);
		out[i].m[0x2] = dot(aRow0, bCol2);
		out[i].m[0x3] = dot(aRow0, bCol3);
		out[i].m[0x4] = dot(aRow1, bCol0);
		out[i].m[0x5] = dot(aRow1, bCol1);
		out[i].m[0x6] = dot(aRow1, bCol2);
		out[i].m[0x7] = dot(aRow1, bCol3);
		out[i].m[0x8] = dot(aRow2, bCol0);
		out[i].m[0x9] = dot(aRow2, bCol1);
		out[i].m[0xa] = dot(aRow2, bCol2);
		out[i].m[0xb] = dot(aRow2, bCol3);
		out[i].m[0xc] = dot(aRow3, bCol0);
		out[i].m[0xd] = dot(aRow3, bCol1);
		out[i].m[0xe] = dot(aRow3, bCol2);
		out[i].m[0xf] = dot(aRow3, bCol3);
	}	
}

export void Mat44_Inverse(uniform int num, uniform Mat44 out[], uniform const Mat44 a[])
{
	for(uniform int i = 0; i < num ; ++i)
	{
		uniform Mat44 mat = out[i];

		uniform float det, invDet;

		uniform float<18> det2 = {
			mat.m[0x0] * mat.m[0x5] - mat.m[0x1] * mat.m[0x4],
			mat.m[0x0] * mat.m[0x6] - mat.m[0x2] * mat.m[0x4],
			mat.m[0x0] * mat.m[0x7] - mat.m[0x3] * mat.m[0x4],
			mat.m[0x1] * mat.m[0x6] - mat.m[0x2] * mat.m[0x5],
			mat.m[0x1] * mat.m[0x7] - mat.m[0x3] * mat.m[0x5],
			mat.m[0x2] * mat.m[0x7] - mat.m[0x3] * mat.m[0x6],
			mat.m[0x0] * mat.m[0xd] - mat.m[0x1] * mat.m[0xc],
			mat.m[0x0] * mat.m[0xe] - mat.m[0x2] * mat.m[0xc],
			mat.m[0x0] * mat.m[0xf] - mat.m[0x3] * mat.m[0xc],
			mat.m[0x1] * mat.m[0xe] - mat.m[0x2] * mat.m[0xd],
			mat.m[0x1] * mat.m[0xf] - mat.m[0x3] * mat.m[0xd],
			mat.m[0x2] * mat.m[0xf] - mat.m[0x3] * mat.m[0xe],
			mat.m[0x4] * mat.m[0xd] - mat.m[0x5] * mat.m[0xc],
			mat.m[0x4] * mat.m[0xe] - mat.m[0x6] * mat.m[0xc],
			mat.m[0x4] * mat.m[0xf] - mat.m[0x7] * mat.m[0xc],
			mat.m[0x5] * mat.m[0xe] - mat.m[0x6] * mat.m[0xd],
			mat.m[0x5] * mat.m[0xf] - mat.m[0x7] * mat.m[0xd],
			mat.m[0x6] * mat.m[0xf] - mat.m[0x7] * mat.m[0xe],
		};

		uniform float<16> det3 = {
			mat.m[0x8] * det2[3] - mat.m[0x9] * det2[1] + mat.m[0xa] * det2[0],
			mat.m[0x8] * det2[4] - mat.m[0x9] * det2[2] + mat.m[0xb] * det2[0],
			mat.m[0x8] * det2[5] - mat.m[0xa] * det2[2] + mat.m[0xb] * det2[1],
			mat.m[0x9] * det2[5] - mat.m[0xa] * det2[4] + mat.m[0xb] * det2[3],
			mat.m[0x8] * det2[9] - mat.m[0x9] * det2[7] + mat.m[0xa] * det2[6],
			mat.m[0x8] * det2[10] - mat.m[0x9] * det2[8] + mat.m[0xb] * det2[6],
			mat.m[0x8] * det2[11] - mat.m[0xa] * det2[8] + mat.m[0xb] * det2[7],
			mat.m[0x9] * det2[11] - mat.m[0xa] * det2[10] + mat.m[0xb] * det2[9],
			mat.m[0x8] * det2[15] - mat.m[0x9] * det2[13] + mat.m[0xa] * det2[12],
			mat.m[0x8] * det2[16] - mat.m[0x9] * det2[14] + mat.m[0xb] * det2[12],
			mat.m[0x8] * det2[17] - mat.m[0xa] * det2[14] + mat.m[0xb] * det2[13],
			mat.m[0x9] * det2[17] - mat.m[0xa] * det2[16] + mat.m[0xb] * det2[15],
			mat.m[0xc] * det2[3] - mat.m[0xd] * det2[1] + mat.m[0xe] * det2[0],
			mat.m[0xc] * det2[4] - mat.m[0xd] * det2[2] + mat.m[0xf] * det2[0],
			mat.m[0xc] * det2[5] - mat.m[0xe] * det2[2] + mat.m[0xf] * det2[1],
			mat.m[0xd] * det2[5] - mat.m[0xe] * det2[4] + mat.m[0xf] * det2[3],
		};

		det = (-det3[3] * mat.m[0xc] + det3[2] * mat.m[0xd] - det3[1] * mat.m[0xe] +
				det3[0] * mat.m[0xf]);

		invDet = 1.0 / det;

		out[i].m[0x0] = -det3[11] * invDet;
		out[i].m[0x1] = det3[7] * invDet;
		out[i].m[0x2] = det3[15] * invDet;
		out[i].m[0x3] = -det3[3] * invDet;

		out[i].m[0x4] = det3[10] * invDet;
		out[i].m[0x5] = -det3[6] * invDet;
		out[i].m[0x6] = -det3[14] * invDet;
		out[i].m[0x7] = det3[2] * invDet;

		out[i].m[0x8] = -det3[9] * invDet;
		out[i].m[0x9] = det3[5] * invDet;
		out[i].m[0xa] = det3[13] * invDet;
		out[i].m[0xb] = -det3[1] * invDet;

		out[i].m[0xc] = det3[8] * invDet;
		out[i].m[0xd] = -det3[4] * invDet;
		out[i].m[0xe] = -det3[12] * invDet;
		out[i].m[0xf] = det3[0] * invDet;
	}
}