#pragma once

#include "core/types.h"
#include "math/vec3.h"

#define DECLARE_SWIZZLE3( T, X, Y, Z ) inline T X ## Y ## Z() const { return T( X, Y, Z ); }

//////////////////////////////////////////////////////////////////////////
// Vec4
struct Vec4
{
public:
	f32 x, y, z, w;

public:
	Vec4() = default;
	Vec4( const Vec2& Rhs );
	Vec4( const Vec3& Rhs, f32 W );
	Vec4( f32* Val );
	Vec4( f32 X, f32 Y, f32 Z, f32 W );
	Vec4( const char* pString );

	Vec4 operator + ( const Vec4& Rhs ) const;
	Vec4 operator - ( const Vec4& Rhs ) const;
	Vec4 operator * ( f32 Rhs ) const;
	Vec4 operator / ( f32 Rhs ) const;
	Vec4 operator * ( const Vec4& Rhs ) const;
	Vec4 operator / ( const Vec4& Rhs ) const;
	Vec4& operator += ( const Vec4& Rhs );
	Vec4& operator -= ( const Vec4& Rhs );
	Vec4& operator *= ( f32 Rhs );
	Vec4& operator /= ( f32 Rhs );

	Vec4 operator - () const;
	f32 magnitude() const;
	f32 magnitudeSquared() const;
	f32 dot( const Vec4& Rhs ) const;
	void normalise();
	void normalise3();
	Vec4 normal() const;
	Vec3 normal3() const;

	bool operator == (const Vec4& Rhs) const;	
	bool operator != (const Vec4& Rhs) const;

	DECLARE_SWIZZLE2( Vec2, x, x );
	DECLARE_SWIZZLE2( Vec2, x, y );
	DECLARE_SWIZZLE2( Vec2, x, z );
	DECLARE_SWIZZLE2( Vec2, x, w );
	DECLARE_SWIZZLE2( Vec2, y, x );
	DECLARE_SWIZZLE2( Vec2, y, y );
	DECLARE_SWIZZLE2( Vec2, y, z );
	DECLARE_SWIZZLE2( Vec2, y, w );
	DECLARE_SWIZZLE2( Vec2, z, x );
	DECLARE_SWIZZLE2( Vec2, z, y );
	DECLARE_SWIZZLE2( Vec2, z, z );
	DECLARE_SWIZZLE2( Vec2, z, w );
	DECLARE_SWIZZLE2( Vec2, w, x );
	DECLARE_SWIZZLE2( Vec2, w, y );
	DECLARE_SWIZZLE2( Vec2, w, z );
	DECLARE_SWIZZLE2( Vec2, w, w );

	DECLARE_SWIZZLE3( Vec3, w, w, w );
	DECLARE_SWIZZLE3( Vec3, w, w, x );
	DECLARE_SWIZZLE3( Vec3, w, w, y );
	DECLARE_SWIZZLE3( Vec3, w, w, z );
	DECLARE_SWIZZLE3( Vec3, w, x, w );
	DECLARE_SWIZZLE3( Vec3, w, x, x );
	DECLARE_SWIZZLE3( Vec3, w, x, y );
	DECLARE_SWIZZLE3( Vec3, w, x, z );
	DECLARE_SWIZZLE3( Vec3, w, y, w );
	DECLARE_SWIZZLE3( Vec3, w, y, x );
	DECLARE_SWIZZLE3( Vec3, w, y, y );
	DECLARE_SWIZZLE3( Vec3, w, y, z );
	DECLARE_SWIZZLE3( Vec3, w, z, w );
	DECLARE_SWIZZLE3( Vec3, w, z, x );
	DECLARE_SWIZZLE3( Vec3, w, z, y );
	DECLARE_SWIZZLE3( Vec3, w, z, z );
	DECLARE_SWIZZLE3( Vec3, x, w, w );
	DECLARE_SWIZZLE3( Vec3, x, w, x );
	DECLARE_SWIZZLE3( Vec3, x, w, y );
	DECLARE_SWIZZLE3( Vec3, x, w, z );
	DECLARE_SWIZZLE3( Vec3, x, x, w );
	DECLARE_SWIZZLE3( Vec3, x, x, x );
	DECLARE_SWIZZLE3( Vec3, x, x, y );
	DECLARE_SWIZZLE3( Vec3, x, x, z );
	DECLARE_SWIZZLE3( Vec3, x, y, w );
	DECLARE_SWIZZLE3( Vec3, x, y, x );
	DECLARE_SWIZZLE3( Vec3, x, y, y );
	DECLARE_SWIZZLE3( Vec3, x, y, z );
	DECLARE_SWIZZLE3( Vec3, x, z, w );
	DECLARE_SWIZZLE3( Vec3, x, z, x );
	DECLARE_SWIZZLE3( Vec3, x, z, y );
	DECLARE_SWIZZLE3( Vec3, x, z, z );
	DECLARE_SWIZZLE3( Vec3, y, w, w );
	DECLARE_SWIZZLE3( Vec3, y, w, x );
	DECLARE_SWIZZLE3( Vec3, y, w, y );
	DECLARE_SWIZZLE3( Vec3, y, w, z );
	DECLARE_SWIZZLE3( Vec3, y, x, w );
	DECLARE_SWIZZLE3( Vec3, y, x, x );
	DECLARE_SWIZZLE3( Vec3, y, x, y );
	DECLARE_SWIZZLE3( Vec3, y, x, z );
	DECLARE_SWIZZLE3( Vec3, y, y, w );
	DECLARE_SWIZZLE3( Vec3, y, y, x );
	DECLARE_SWIZZLE3( Vec3, y, y, y );
	DECLARE_SWIZZLE3( Vec3, y, y, z );
	DECLARE_SWIZZLE3( Vec3, y, z, w );
	DECLARE_SWIZZLE3( Vec3, y, z, x );
	DECLARE_SWIZZLE3( Vec3, y, z, y );
	DECLARE_SWIZZLE3( Vec3, y, z, z );
	DECLARE_SWIZZLE3( Vec3, z, w, w );
	DECLARE_SWIZZLE3( Vec3, z, w, x );
	DECLARE_SWIZZLE3( Vec3, z, w, y );
	DECLARE_SWIZZLE3( Vec3, z, w, z );
	DECLARE_SWIZZLE3( Vec3, z, x, w );
	DECLARE_SWIZZLE3( Vec3, z, x, x );
	DECLARE_SWIZZLE3( Vec3, z, x, y );
	DECLARE_SWIZZLE3( Vec3, z, x, z );
	DECLARE_SWIZZLE3( Vec3, z, y, w );
	DECLARE_SWIZZLE3( Vec3, z, y, x );
	DECLARE_SWIZZLE3( Vec3, z, y, y );
	DECLARE_SWIZZLE3( Vec3, z, y, z );
	DECLARE_SWIZZLE3( Vec3, z, z, w );
	DECLARE_SWIZZLE3( Vec3, z, z, x );
	DECLARE_SWIZZLE3( Vec3, z, z, y );
	DECLARE_SWIZZLE3( Vec3, z, z, z );
};

//////////////////////////////////////////////////////////////////////////
// Inlines
FORCEINLINE Vec4::Vec4( f32* Val ):
	x( Val[0] ),
	y( Val[1] ),
	z( Val[2] ),
	w( Val[3] )
{
}

FORCEINLINE Vec4::Vec4( f32 X, f32 Y, f32 Z, f32 W ):
	x( X ),
	y( Y ),
	z( Z ),
	w( W )
{

}

FORCEINLINE Vec4 Vec4::operator + ( const Vec4& Rhs ) const
{
	return Vec4( x + Rhs.x, y + Rhs.y, z + Rhs.z, w + Rhs.w );
}

FORCEINLINE Vec4 Vec4::operator - ( const Vec4& Rhs ) const
{ 
	return Vec4( x - Rhs.x, y - Rhs.y, z - Rhs.z, w - Rhs.w );				
}

FORCEINLINE Vec4 Vec4::operator * ( f32 Rhs ) const
{
	return Vec4( x * Rhs, y * Rhs, z * Rhs, w * Rhs );
}


FORCEINLINE Vec4 Vec4::operator / ( const Vec4& Rhs ) const
{ 
	return Vec4( x / Rhs.x, y / Rhs.y, z / Rhs.z, w / Rhs.w );					
}

FORCEINLINE Vec4 Vec4::operator * ( const Vec4& Rhs ) const
{
	return Vec4( x * Rhs.x, y * Rhs.y, z * Rhs.z, w * Rhs.w );
}


FORCEINLINE Vec4 Vec4::operator / ( f32 Rhs ) const
{ 
	const f32 InvRhs = 1.0f / Rhs;
	return Vec4( x * InvRhs, y * InvRhs, z * InvRhs, w * InvRhs );					
}

FORCEINLINE Vec4& Vec4::operator += ( const Vec4& Rhs ) 
{ 
	x += Rhs.x;
	y += Rhs.y;
	z += Rhs.z;
	w += Rhs.w;
	return (*this);			
}

FORCEINLINE Vec4& Vec4::operator -= (const Vec4& Rhs) 
{ 
	x -= Rhs.x;
	y -= Rhs.y;
	z -= Rhs.z;
	w -= Rhs.w;
	return (*this);			
}

FORCEINLINE Vec4& Vec4::operator *= ( f32 Rhs ) 	
{ 
	x *= Rhs;
	y *= Rhs;
	z *= Rhs;
	w *= Rhs;
	return (*this);				
}

FORCEINLINE Vec4& Vec4::operator /= ( f32 Rhs ) 	
{ 
	const f32 InvRhs = 1.0f / Rhs;
	x *= InvRhs;
	y *= InvRhs;
	z *= InvRhs;
	w *= InvRhs;
	return (*this);				
}

FORCEINLINE Vec4 Vec4::operator - () const
{
	return Vec4( -x, -y, -z, -w );
}

FORCEINLINE f32 Vec4::magnitudeSquared() const
{
	return dot(*this);
}

FORCEINLINE f32 Vec4::dot( const Vec4& Rhs ) const
{
	return ( x* Rhs.x )+( y * Rhs.y )+( z* Rhs.z )+( w * Rhs.w );
}

FORCEINLINE bool Vec4::operator != ( const Vec4 &Rhs ) const
{
	return !( (*this) == Rhs );
}

bool CheckFloat( Vec4 T );
