#include "math/mat33.h"
#include "math/mat44.h"

#include "math/vec2.h"
#include "math/vec3.h"

#include <cmath>

Mat44 Mat44::operator+(const Mat44& Rhs)
{
	return Mat44(Row0_ + Rhs.Row0_, Row1_ + Rhs.Row1_, Row2_ + Rhs.Row2_, Row3_ + Rhs.Row3_);
}

Mat44 Mat44::operator-(const Mat44& Rhs)
{
	return Mat44(Row0_ - Rhs.Row0_, Row1_ - Rhs.Row1_, Row2_ - Rhs.Row2_, Row3_ - Rhs.Row3_);
}

Mat44 Mat44::operator*(f32 Rhs)
{
	return Mat44(Row0_ * Rhs, Row1_ * Rhs, Row2_ * Rhs, Row3_ * Rhs);
}

Mat44 Mat44::operator/(f32 Rhs)
{
	return Mat44(Row0_ / Rhs, Row1_ / Rhs, Row2_ / Rhs, Row3_ / Rhs);
}

Mat44 Mat44::operator*(const Mat44& Rhs) const
{
	const Mat44& Lhs = (*this);

	return Mat44(Lhs[0][0] * Rhs[0][0] + Lhs[0][1] * Rhs[1][0] + Lhs[0][2] * Rhs[2][0] + Lhs[0][3] * Rhs[3][0],
	    Lhs[0][0] * Rhs[0][1] + Lhs[0][1] * Rhs[1][1] + Lhs[0][2] * Rhs[2][1] + Lhs[0][3] * Rhs[3][1],
	    Lhs[0][0] * Rhs[0][2] + Lhs[0][1] * Rhs[1][2] + Lhs[0][2] * Rhs[2][2] + Lhs[0][3] * Rhs[3][2],
	    Lhs[0][0] * Rhs[0][3] + Lhs[0][1] * Rhs[1][3] + Lhs[0][2] * Rhs[2][3] + Lhs[0][3] * Rhs[3][3],
	    Lhs[1][0] * Rhs[0][0] + Lhs[1][1] * Rhs[1][0] + Lhs[1][2] * Rhs[2][0] + Lhs[1][3] * Rhs[3][0],
	    Lhs[1][0] * Rhs[0][1] + Lhs[1][1] * Rhs[1][1] + Lhs[1][2] * Rhs[2][1] + Lhs[1][3] * Rhs[3][1],
	    Lhs[1][0] * Rhs[0][2] + Lhs[1][1] * Rhs[1][2] + Lhs[1][2] * Rhs[2][2] + Lhs[1][3] * Rhs[3][2],
	    Lhs[1][0] * Rhs[0][3] + Lhs[1][1] * Rhs[1][3] + Lhs[1][2] * Rhs[2][3] + Lhs[1][3] * Rhs[3][3],
	    Lhs[2][0] * Rhs[0][0] + Lhs[2][1] * Rhs[1][0] + Lhs[2][2] * Rhs[2][0] + Lhs[2][3] * Rhs[3][0],
	    Lhs[2][0] * Rhs[0][1] + Lhs[2][1] * Rhs[1][1] + Lhs[2][2] * Rhs[2][1] + Lhs[2][3] * Rhs[3][1],
	    Lhs[2][0] * Rhs[0][2] + Lhs[2][1] * Rhs[1][2] + Lhs[2][2] * Rhs[2][2] + Lhs[2][3] * Rhs[3][2],
	    Lhs[2][0] * Rhs[0][3] + Lhs[2][1] * Rhs[1][3] + Lhs[2][2] * Rhs[2][3] + Lhs[2][3] * Rhs[3][3],
	    Lhs[3][0] * Rhs[0][0] + Lhs[3][1] * Rhs[1][0] + Lhs[3][2] * Rhs[2][0] + Lhs[3][3] * Rhs[3][0],
	    Lhs[3][0] * Rhs[0][1] + Lhs[3][1] * Rhs[1][1] + Lhs[3][2] * Rhs[2][1] + Lhs[3][3] * Rhs[3][1],
	    Lhs[3][0] * Rhs[0][2] + Lhs[3][1] * Rhs[1][2] + Lhs[3][2] * Rhs[2][2] + Lhs[3][3] * Rhs[3][2],
	    Lhs[3][0] * Rhs[0][3] + Lhs[3][1] * Rhs[1][3] + Lhs[3][2] * Rhs[2][3] + Lhs[3][3] * Rhs[3][3]);
}

f32 Mat44::Determinant()
{
	const Mat44& Lhs = (*this);

	const Mat33 A = Mat33(Vec3(Lhs[1][1], Lhs[1][2], Lhs[1][3]), Vec3(Lhs[2][1], Lhs[2][2], Lhs[2][3]),
	    Vec3(Lhs[3][1], Lhs[3][2], Lhs[3][3]));
	const Mat33 B = Mat33(Vec3(Lhs[1][0], Lhs[1][2], Lhs[1][3]), Vec3(Lhs[2][0], Lhs[2][2], Lhs[2][3]),
	    Vec3(Lhs[3][0], Lhs[3][2], Lhs[3][3]));
	const Mat33 C = Mat33(Vec3(Lhs[1][0], Lhs[1][1], Lhs[1][3]), Vec3(Lhs[2][0], Lhs[2][1], Lhs[2][3]),
	    Vec3(Lhs[3][0], Lhs[3][1], Lhs[3][3]));
	const Mat33 D = Mat33(Vec3(Lhs[1][0], Lhs[1][1], Lhs[1][2]), Vec3(Lhs[2][0], Lhs[2][1], Lhs[2][2]),
	    Vec3(Lhs[3][0], Lhs[3][1], Lhs[3][2]));

	return ((A.Determinant() * Lhs[0][0]) - (B.Determinant() * Lhs[0][1]) + (C.Determinant() * Lhs[0][2]) -
	        (D.Determinant() * Lhs[0][3]));
}

void Mat44::Rotation(const Vec3& Angles)
{
	f32 sy, sp, sr;
	f32 cy, cp, cr;

	sy = sinf(Angles.y);
	sp = sinf(Angles.x);
	sr = sinf(Angles.z);

	cy = cosf(Angles.y);
	cp = cosf(Angles.x);
	cr = cosf(Angles.z);

	Row0_ = Vec4(cy * cr + sy * sp * sr, -cy * sr + sy * sp * cr, sy * cp, 0.0f);
	Row1_ = Vec4(sr * cp, cr * cp, -sp, 0.0f);
	Row2_ = Vec4(-sy * cr + cy * sp * sr, sr * sy + cy * sp * cr, cy * cp, 0.0f);
}

void Mat44::Translation(const Vec3& translation)
{
	Translation(Vec4(translation.x, translation.y, translation.z, 1.0f));
}

void Mat44::Translation(const Vec4& Translation)
{
	Row3(Translation);
}


void Mat44::Scale(const Vec3& scale)
{
	Scale(Vec4(scale.x, scale.y, scale.z, 1.0f));
}

void Mat44::Scale(const Vec4& scale)
{
	Row0(Vec4(scale.x, 0.0f, 0.0f, 0.0f));
	Row1(Vec4(0.0f, scale.y, 0.0f, 0.0f));
	Row2(Vec4(0.0f, 0.0f, scale.z, 0.0f));
	Row3(Vec4(0.0f, 0.0f, 0.0f, scale.w));
}

void Mat44::Inverse()
{
	const Mat44& Lhs = (*this);
	f32 Det, InvDet;

	const f32 Det2_01_01 = Lhs[0][0] * Lhs[1][1] - Lhs[0][1] * Lhs[1][0];
	const f32 Det2_01_02 = Lhs[0][0] * Lhs[1][2] - Lhs[0][2] * Lhs[1][0];
	const f32 Det2_01_03 = Lhs[0][0] * Lhs[1][3] - Lhs[0][3] * Lhs[1][0];
	const f32 Det2_01_12 = Lhs[0][1] * Lhs[1][2] - Lhs[0][2] * Lhs[1][1];
	const f32 Det2_01_13 = Lhs[0][1] * Lhs[1][3] - Lhs[0][3] * Lhs[1][1];
	const f32 Det2_01_23 = Lhs[0][2] * Lhs[1][3] - Lhs[0][3] * Lhs[1][2];

	const f32 Det3_201_012 = Lhs[2][0] * Det2_01_12 - Lhs[2][1] * Det2_01_02 + Lhs[2][2] * Det2_01_01;
	const f32 Det3_201_013 = Lhs[2][0] * Det2_01_13 - Lhs[2][1] * Det2_01_03 + Lhs[2][3] * Det2_01_01;
	const f32 Det3_201_023 = Lhs[2][0] * Det2_01_23 - Lhs[2][2] * Det2_01_03 + Lhs[2][3] * Det2_01_02;
	const f32 Det3_201_123 = Lhs[2][1] * Det2_01_23 - Lhs[2][2] * Det2_01_13 + Lhs[2][3] * Det2_01_12;

	Det = (-Det3_201_123 * Lhs[3][0] + Det3_201_023 * Lhs[3][1] - Det3_201_013 * Lhs[3][2] + Det3_201_012 * Lhs[3][3]);

	InvDet = 1.0f / Det;

	const f32 Det2_03_01 = Lhs[0][0] * Lhs[3][1] - Lhs[0][1] * Lhs[3][0];
	const f32 Det2_03_02 = Lhs[0][0] * Lhs[3][2] - Lhs[0][2] * Lhs[3][0];
	const f32 Det2_03_03 = Lhs[0][0] * Lhs[3][3] - Lhs[0][3] * Lhs[3][0];
	const f32 Det2_03_12 = Lhs[0][1] * Lhs[3][2] - Lhs[0][2] * Lhs[3][1];
	const f32 Det2_03_13 = Lhs[0][1] * Lhs[3][3] - Lhs[0][3] * Lhs[3][1];
	const f32 Det2_03_23 = Lhs[0][2] * Lhs[3][3] - Lhs[0][3] * Lhs[3][2];

	const f32 Det2_13_01 = Lhs[1][0] * Lhs[3][1] - Lhs[1][1] * Lhs[3][0];
	const f32 Det2_13_02 = Lhs[1][0] * Lhs[3][2] - Lhs[1][2] * Lhs[3][0];
	const f32 Det2_13_03 = Lhs[1][0] * Lhs[3][3] - Lhs[1][3] * Lhs[3][0];
	const f32 Det2_13_12 = Lhs[1][1] * Lhs[3][2] - Lhs[1][2] * Lhs[3][1];
	const f32 Det2_13_13 = Lhs[1][1] * Lhs[3][3] - Lhs[1][3] * Lhs[3][1];
	const f32 Det2_13_23 = Lhs[1][2] * Lhs[3][3] - Lhs[1][3] * Lhs[3][2];

	const f32 Det3_203_012 = Lhs[2][0] * Det2_03_12 - Lhs[2][1] * Det2_03_02 + Lhs[2][2] * Det2_03_01;
	const f32 Det3_203_013 = Lhs[2][0] * Det2_03_13 - Lhs[2][1] * Det2_03_03 + Lhs[2][3] * Det2_03_01;
	const f32 Det3_203_023 = Lhs[2][0] * Det2_03_23 - Lhs[2][2] * Det2_03_03 + Lhs[2][3] * Det2_03_02;
	const f32 Det3_203_123 = Lhs[2][1] * Det2_03_23 - Lhs[2][2] * Det2_03_13 + Lhs[2][3] * Det2_03_12;

	const f32 Det3_213_012 = Lhs[2][0] * Det2_13_12 - Lhs[2][1] * Det2_13_02 + Lhs[2][2] * Det2_13_01;
	const f32 Det3_213_013 = Lhs[2][0] * Det2_13_13 - Lhs[2][1] * Det2_13_03 + Lhs[2][3] * Det2_13_01;
	const f32 Det3_213_023 = Lhs[2][0] * Det2_13_23 - Lhs[2][2] * Det2_13_03 + Lhs[2][3] * Det2_13_02;
	const f32 Det3_213_123 = Lhs[2][1] * Det2_13_23 - Lhs[2][2] * Det2_13_13 + Lhs[2][3] * Det2_13_12;

	const f32 Det3_301_012 = Lhs[3][0] * Det2_01_12 - Lhs[3][1] * Det2_01_02 + Lhs[3][2] * Det2_01_01;
	const f32 Det3_301_013 = Lhs[3][0] * Det2_01_13 - Lhs[3][1] * Det2_01_03 + Lhs[3][3] * Det2_01_01;
	const f32 Det3_301_023 = Lhs[3][0] * Det2_01_23 - Lhs[3][2] * Det2_01_03 + Lhs[3][3] * Det2_01_02;
	const f32 Det3_301_123 = Lhs[3][1] * Det2_01_23 - Lhs[3][2] * Det2_01_13 + Lhs[3][3] * Det2_01_12;

	Row0_.x = -Det3_213_123 * InvDet;
	Row1_.x = Det3_213_023 * InvDet;
	Row2_.x = -Det3_213_013 * InvDet;
	Row3_.x = Det3_213_012 * InvDet;

	Row0_.y = Det3_203_123 * InvDet;
	Row1_.y = -Det3_203_023 * InvDet;
	Row2_.y = Det3_203_013 * InvDet;
	Row3_.y = -Det3_203_012 * InvDet;

	Row0_.z = Det3_301_123 * InvDet;
	Row1_.z = -Det3_301_023 * InvDet;
	Row2_.z = Det3_301_013 * InvDet;
	Row3_.z = -Det3_301_012 * InvDet;

	Row0_.w = -Det3_201_123 * InvDet;
	Row1_.w = Det3_201_023 * InvDet;
	Row2_.w = -Det3_201_013 * InvDet;
	Row3_.w = Det3_201_012 * InvDet;
}

void Mat44::LookAt(const Vec3& position, const Vec3& lookAt, const Vec3& upVec)
{
	const Vec3 front = (position - lookAt).Normal();
	const Vec3 side = front.Cross(upVec).Normal();
	const Vec3 up = side.Cross(front).Normal();

	Mat44 rotMatrix(Vec4(side.x, up.x, -front.x, 0.0f), Vec4(side.y, up.y, -front.y, 0.0f),
	    Vec4(side.z, up.z, -front.z, 0.0f), Vec4(0.0f, 0.0f, 0.0f, 1.0f));


	Mat44 transMatrix(Vec4(1.0f, 0.0f, 0.0f, 0.0f), Vec4(0.0f, 1.0f, 0.0f, 0.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f),
	    Vec4(-position.x, -position.y, -position.z, 1.0f));

	(*this) = transMatrix * rotMatrix;
}

void Mat44::OrthoProjection(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
	// TODO: Optimise.
	Mat44& projection = (*this);

	projection[0][0] = 2.0f / (right - left);
	projection[0][1] = 0.0f;
	projection[0][2] = 0.0f;
	projection[0][3] = 0.0f;

	projection[1][0] = 0.0f;
	projection[1][1] = 2.0f / (top - bottom);
	projection[1][2] = 0.0f;
	projection[1][3] = 0.0f;

	projection[2][0] = 0.0f;
	projection[2][1] = 0.0f;
	projection[2][2] = 2.0f / (far - near);
	projection[2][3] = 0.0f;

	projection[3][0] = -(right + left) / (right - left);
	projection[3][1] = -(top + bottom) / (top - bottom);
	projection[3][2] = -(far + near) / (far - near);
	projection[3][3] = 1.0f;
}

void Mat44::PerspProjectionHorizontal(f32 Fov, f32 Aspect, f32 Near, f32 Far)
{
	const f32 W = tanf(Fov) * Near;
	const f32 H = W / Aspect;

	Frustum(-W, W, H, -H, Near, Far);
}

void Mat44::PerspProjectionVertical(f32 Fov, f32 Aspect, f32 Near, f32 Far)
{
	const f32 H = tanf(Fov) * Near;
	const f32 W = H / Aspect;

	Frustum(-W, W, H, -H, Near, Far);
}

void Mat44::Frustum(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
	// TODO: Optimise.
	Mat44& projection = (*this);

	projection[0][0] = (2.0f * near) / (right - left);
	projection[0][1] = 0.0f;
	projection[0][2] = 0.0f;
	projection[0][3] = 0.0f;

	projection[1][0] = 0.0f;
	projection[1][1] = (2.0f * near) / (bottom - top);
	projection[1][2] = 0.0f;
	projection[1][3] = 0.0f;

	projection[2][0] = 0.0f;
	projection[2][1] = 0.0f;
	projection[2][2] = (far + near) / (far - near);
	projection[2][3] = 1.0f;

	projection[3][0] = 0.0f;
	projection[3][1] = 0.0f;
	projection[3][2] = -(2.0f * far * near) / (far - near);
	projection[3][3] = 0.0f;
}

bool Mat44::operator==(const Mat44& Other) const
{
	return Row0_ == Other.Row0_ && Row1_ == Other.Row1_ && Row2_ == Other.Row2_ && Row3_ == Other.Row3_;
}

bool Mat44::IsIdentity() const
{
	static Mat44 Identity;
	return (*this) == Identity;
}

Vec2 operator*(const Vec2& Lhs, const Mat44& Rhs)
{
	return Vec2(Lhs.x * Rhs[0][0] + Lhs.y * Rhs[1][0] + Rhs[3][0], Lhs.x * Rhs[0][1] + Lhs.y * Rhs[1][1] + Rhs[3][1]);
}

Vec3 operator*(const Vec3& Lhs, const Mat44& Rhs)
{
	return Vec3(Lhs.x * Rhs[0][0] + Lhs.y * Rhs[1][0] + Lhs.z * Rhs[2][0] + Rhs[3][0],
	    Lhs.x * Rhs[0][1] + Lhs.y * Rhs[1][1] + Lhs.z * Rhs[2][1] + Rhs[3][1],
	    Lhs.x * Rhs[0][2] + Lhs.y * Rhs[1][2] + Lhs.z * Rhs[2][2] + Rhs[3][2]);
}

Vec4 operator*(const Vec4& Lhs, const Mat44& Rhs)
{
	return Vec4(Lhs.x * Rhs[0][0] + Lhs.y * Rhs[1][0] + Lhs.z * Rhs[2][0] + Lhs.w * Rhs[3][0],
	    Lhs.x * Rhs[0][1] + Lhs.y * Rhs[1][1] + Lhs.z * Rhs[2][1] + Lhs.w * Rhs[3][1],
	    Lhs.x * Rhs[0][2] + Lhs.y * Rhs[1][2] + Lhs.z * Rhs[2][2] + Lhs.w * Rhs[3][2],
	    Lhs.x * Rhs[0][3] + Lhs.y * Rhs[1][3] + Lhs.z * Rhs[2][3] + Lhs.w * Rhs[3][3]);
}

Vec3 Mat44::Translation() const
{
	return Vec3(Row3_.x, Row3_.y, Row3_.z);
}