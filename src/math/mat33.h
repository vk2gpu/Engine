#pragma once

#include "math/dll.h"
#include "math/vec3.h"

namespace Math
{
	/**
	 * 3x3 Matrix.
	 */
	class MATH_DLL Mat33 final
	{
	public:
		Mat33() {}
		Mat33(const Vec3&, const Vec3&, const Vec3&);

		Mat33(f32 I00, f32 I01, f32 I02, f32 I10, f32 I11, f32 I12, f32 I20, f32 I21, f32 I22);

		// Accessor
		f32* operator[](u32 i) { return reinterpret_cast<f32*>(&Row0_) + (i * 3); }
		const f32* operator[](u32 i) const { return reinterpret_cast<const f32*>(&Row0_) + (i * 3); }

		const Vec3& Row0() const { return Row0_; }
		const Vec3& Row1() const { return Row1_; }
		const Vec3& Row2() const { return Row2_; }

		void Row0(const Vec3& Row) { Row0_ = Row; }
		void Row1(const Vec3& Row) { Row1_ = Row; }
		void Row2(const Vec3& Row) { Row2_ = Row; }

		// Arithmetic
		Mat33 operator+(const Mat33& Rhs) { return Mat33(Row0_ + Rhs.Row0_, Row1_ + Rhs.Row1_, Row2_ + Rhs.Row2_); }
		Mat33 operator-(const Mat33& Rhs) { return Mat33(Row0_ - Rhs.Row0_, Row1_ - Rhs.Row1_, Row2_ - Rhs.Row2_); }
		Mat33 operator*(f32 Rhs) { return Mat33(Row0_ * Rhs, Row1_ * Rhs, Row2_ * Rhs); }
		Mat33 operator/(f32 Rhs) { return Mat33(Row0_ / Rhs, Row1_ / Rhs, Row2_ / Rhs); }
		Mat33 operator*(const Mat33& rhs);

		void Identity();
		void Transpose();

		f32 Determinant() const;

	private:
		Vec3 Row0_;
		Vec3 Row1_;
		Vec3 Row2_;
	};

	inline void Mat33::Identity()
	{
		Row0_ = Vec3(1.0f, 0.0f, 0.0f);
		Row1_ = Vec3(0.0f, 1.0f, 0.0f);
		Row2_ = Vec3(0.0f, 0.0f, 1.0f);
	}

	inline void Mat33::Transpose()
	{
		const Mat33& Lhs = (*this);

		(*this) = Mat33(Vec3(Lhs[0][0], Lhs[1][0], Lhs[2][0]), Vec3(Lhs[0][1], Lhs[1][1], Lhs[2][1]),
		    Vec3(Lhs[0][2], Lhs[1][2], Lhs[2][2]));
	}

	//
	inline Vec3 operator*(const Vec3& Lhs, const Mat33& Rhs)
	{
		return Vec3(Lhs.x * Rhs[0][0] + Lhs.y * Rhs[1][0] + Lhs.z * Rhs[2][0],
		    Lhs.x * Rhs[0][1] + Lhs.y * Rhs[1][1] + Lhs.z * Rhs[2][1],
		    Lhs.x * Rhs[0][2] + Lhs.y * Rhs[1][2] + Lhs.z * Rhs[2][2]);
	}
} // namespace Math
