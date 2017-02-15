#include "Math/Mat33.h"

Mat33::Mat33(const Vec3& Row0, const Vec3& Row1, const Vec3& Row2)
{
	Row0_ = Row0;
	Row1_ = Row1;
	Row2_ = Row2;
}


Mat33::Mat33(f32 I00, f32 I01, f32 I02, f32 I10, f32 I11, f32 I12, f32 I20, f32 I21, f32 I22)
{
	Row0_ = Vec3(I00, I01, I02);
	Row1_ = Vec3(I10, I11, I12);
	Row2_ = Vec3(I20, I21, I22);
}

Mat33 Mat33::operator+(const Mat33& Rhs)
{
	return Mat33(Row0_ + Rhs.Row0_, Row1_ + Rhs.Row1_, Row2_ + Rhs.Row2_);
}

Mat33 Mat33::operator-(const Mat33& Rhs)
{
	return Mat33(Row0_ - Rhs.Row0_, Row1_ - Rhs.Row1_, Row2_ - Rhs.Row2_);
}

Mat33 Mat33::operator*(f32 Rhs)
{
	return Mat33(Row0_ * Rhs, Row1_ * Rhs, Row2_ * Rhs);
}

Mat33 Mat33::operator/(f32 Rhs)
{
	return Mat33(Row0_ / Rhs, Row1_ / Rhs, Row2_ / Rhs);
}

Mat33 Mat33::operator*(const Mat33& Rhs)
{
	const Mat33& Lhs = (*this);

	return Mat33(Lhs[0][0] * Rhs[0][0] + Lhs[0][1] * Rhs[1][0] + Lhs[0][2] * Rhs[2][0],
	    Lhs[0][0] * Rhs[0][1] + Lhs[0][1] * Rhs[1][1] + Lhs[0][2] * Rhs[2][1],
	    Lhs[0][0] * Rhs[0][2] + Lhs[0][1] * Rhs[1][2] + Lhs[0][2] * Rhs[2][2],
	    Lhs[1][0] * Rhs[0][0] + Lhs[1][1] * Rhs[1][0] + Lhs[1][2] * Rhs[2][0],
	    Lhs[1][0] * Rhs[0][1] + Lhs[1][1] * Rhs[1][1] + Lhs[1][2] * Rhs[2][1],
	    Lhs[1][0] * Rhs[0][2] + Lhs[1][1] * Rhs[1][2] + Lhs[1][2] * Rhs[2][2],
	    Lhs[2][0] * Rhs[0][0] + Lhs[2][1] * Rhs[1][0] + Lhs[2][2] * Rhs[2][0],
	    Lhs[2][0] * Rhs[0][1] + Lhs[2][1] * Rhs[1][1] + Lhs[2][2] * Rhs[2][1],
	    Lhs[2][0] * Rhs[0][2] + Lhs[2][1] * Rhs[1][2] + Lhs[2][2] * Rhs[2][2]);
}

f32 Mat33::Determinant() const
{
	const Mat33& Lhs = (*this);

	return (Lhs[0][0] * Lhs[1][1] * Lhs[2][2]) - (Lhs[0][0] * Lhs[1][2] * Lhs[2][1]) +
	       (Lhs[0][1] * Lhs[1][2] * Lhs[2][0]) - (Lhs[0][1] * Lhs[1][0] * Lhs[2][2]) +
	       (Lhs[0][2] * Lhs[1][0] * Lhs[2][1]) - (Lhs[0][2] * Lhs[1][1] * Lhs[2][0]);
}
