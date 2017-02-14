#pragma once

#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat44.h"

//////////////////////////////////////////////////////////////////////////
// Quat
class Quat : public Vec4
{
public:
	// ctor	
	Quat();
	Quat( f32* Val );
	Quat( f32 lX, f32 lY, f32 lZ, f32 lW );
	Quat( const char* pString );
	
	// Arithmetic
	Quat operator * (const Quat& rhs) const;
	Quat operator ~ () const;


	// Additional stuff
	void MakeIdentity();
	f32 Magnitude();
	void Inverse();
	
	// Interpolation
	static Quat Slerp( const Quat& a, const Quat& b, f32 t );
	
	Vec3 RotateVector( const Vec3& ) const;

	void FromAxis( const Vec3& X, const Vec3& Y, const Vec3& Z );

	//
	void FromMatrix4d( const Mat44& Mat );
	void AsMatrix4d( Mat44& Matrix ) const;

	//
	void RotateTo( const Vec3& From, const Vec3& To );
	void AxisAngle( const Vec3& Axis, f32 Angle );
	
	// 
	void FromEuler( f32 Yaw, f32 Pitch, f32 Roll );
	Vec3 AsEuler() const;

	//
	void CalcFromXYZ();
};

// ctor
inline Quat::Quat()
{
	MakeIdentity();
}

inline Quat::Quat( f32* Val ):
	Vec4( Val )
{

}

inline Quat::Quat( f32 lX, f32 lY, f32 lZ, f32 lW ):
	Vec4( lX, lY, lZ, lW )
{

}
