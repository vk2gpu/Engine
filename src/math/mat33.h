#pragma once

#include "Math/Vec3.h"

//////////////////////////////////////////////////////////////////////////
// Mat33
class Mat33
{
public:

	Mat33(){}	
	Mat33( const Vec3&,
			 const Vec3&,
			 const Vec3& );

	Mat33( f32 I00,
			 f32 I01,
			 f32 I02,
			 f32 I10,
			 f32 I11,
			 f32 I12,
			 f32 I20,
			 f32 I21,
			 f32 I22 );

	// Accessor
	f32*			operator [] ( u32 i );
	const f32*	operator [] ( u32 i ) const;

	const Vec3&	Row0() const;
	const Vec3&	Row1() const;
	const Vec3&	Row2() const;

	void Row0( const Vec3& Row );
	void Row1( const Vec3& Row );
	void Row2( const Vec3& Row );

	// Arithmetic
	Mat33		operator + ( const Mat33& rhs );
	Mat33		operator - ( const Mat33& rhs );
	Mat33		operator * ( f32 rhs );
	Mat33		operator / ( f32 rhs );
	Mat33		operator * ( const Mat33& rhs );

	void		Identity();
	void		Transpose();

	f32		Determinant() const;

private:
	Vec3 Row0_;
	Vec3 Row1_;
	Vec3 Row2_;

};

//////////////////////////////////////////////////////////////////////////
// Inlines

inline f32* Mat33::operator [] ( u32 i )
{
	return reinterpret_cast< f32* >( &Row0_ ) + ( i * 3 );
}

inline const f32* Mat33::operator [] ( u32 i ) const
{
	return reinterpret_cast< const f32* >( &Row0_ ) + ( i * 3 );
}

inline const Vec3& Mat33::Row0() const
{
	return Row0_;
}

inline const Vec3& Mat33::Row1() const
{
	return Row1_;
}

inline const Vec3& Mat33::Row2() const
{
	return Row2_;
}

inline void Mat33::Row0( const Vec3& Row0 )
{
	Row0_ = Row0;
}

inline void Mat33::Row1( const Vec3& Row1 )
{
	Row1_ = Row1;
}

inline void Mat33::Row2( const Vec3& Row2 )
{
	Row2_ = Row2;
}

inline void Mat33::Identity()
{
	Row0_ = Vec3( 1.0f, 0.0f, 0.0f );
	Row1_ = Vec3( 0.0f, 1.0f, 0.0f );
	Row2_ = Vec3( 0.0f, 0.0f, 1.0f );
}

inline void Mat33::Transpose()
{
	const Mat33& Lhs = (*this);

	(*this) = Mat33( Vec3( Lhs[0][0], Lhs[1][0], Lhs[2][0] ),
	                   Vec3( Lhs[0][1], Lhs[1][1], Lhs[2][1] ),
	                   Vec3( Lhs[0][2], Lhs[1][2], Lhs[2][2] ) );
}

//
inline Vec3 operator * ( const Vec3& Lhs, const Mat33& Rhs )
{
	return Vec3( Lhs.x * Rhs[0][0] + Lhs.y * Rhs[1][0] + Lhs.z * Rhs[2][0],
	                Lhs.x * Rhs[0][1] + Lhs.y * Rhs[1][1] + Lhs.z * Rhs[2][1],
	                Lhs.x * Rhs[0][2] + Lhs.y * Rhs[1][2] + Lhs.z * Rhs[2][2] );
}
