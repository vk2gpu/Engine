#include "math/plane.h"
#include "math/float.h"

#include <cmath>

void Plane::Transform( const Mat44& Transform )
{
	Vec3 Translation( Transform.Row3().x, Transform.Row3().y, Transform.Row3().z );
	Normal_ = ( Normal_ * Transform ) - Translation;
	D_ = D_ - ( Normal_.Dot( Translation ) );
}

Plane::eClassify Plane::Classify( const Vec3& Point, f32 Radius ) const
{
	f32 Dist = Distance( Point );
			
	if( Dist > Radius )
	{
		return FRONT;
	}
	else if( Dist < -Radius )
	{
		return BACK;
	}

	return COINCIDING;
}

bool Plane::LineIntersection( const Vec3& Point, const Vec3& Dir, f32& Distance ) const
{
	const f32 Dist = ( Normal_.Dot( Point ) ) + D_;
	const f32 Ndiv = Normal_.Dot( -Dir );

	if( std::abs( Ndiv ) > 0.0f )
	{
		Distance = Dist / Ndiv;
		return true;
	}

	return false;
}

bool Plane::LineIntersection( const Vec3& A, const Vec3& B, f32& Distance, Vec3& Intersection ) const
{
	bool RetVal = false;
	Vec3 Dir = B - A;
	if( LineIntersection( A, Dir, Distance ) && Distance >= 0.0f && Distance <= 1.0f )
	{
		Intersection = A + ( Dir * Distance );
		RetVal = true;
	}
	return RetVal;
}

bool Plane::Intersect( const Plane& A, const Plane& B, const Plane& C, Vec3& Point )
{
	const f32 Denom = A.Normal_.Dot( ( B.Normal_.Cross( C.Normal_ ) ) );

	if ( std::abs( Denom ) < ( F32_EPSILON ) )
	{
		return false;
	}

	Point = ( ( ( B.Normal_.Cross( C.Normal_ ) ) * -A.D_ ) -
	          ( ( C.Normal_.Cross( A.Normal_ ) ) *  B.D_ ) -
	          ( ( A.Normal_.Cross( B.Normal_ ) ) *  C.D_ ) ) / Denom;

	return true;
}

void Plane::FromPoints( const Vec3& V1, const Vec3& V2, const Vec3& V3 )
{
	Normal_ = ( V1 - V2 ).Cross( ( V3 - V2 ) );
	Normal_.Normalise();
	D_ = -( V1.Dot( Normal_ ) );
}

void Plane::FromPointNormal( const Vec3& Point, const Vec3& Normal )
{
	Normal_ = Normal;
	Normal_.Normalise();
	D_ = -( Point.Dot( Normal_ ) );
}

f32 Plane::Distance( const Vec3& P ) const
{
	return ( Normal_.Dot(P) ) + D_;
}

void Plane::Normalise()
{
	D_ = D_ / Normal_.Magnitude();
	Normal_.Normalise();
};

const Vec3& Plane::Normal() const
{
	return Normal_;
}

f32 Plane::D() const
{
	return D_;
};

bool Plane::operator == (const Plane& Other ) const
{
	return ( Other.Normal() == Normal() && Other.D() == D() );
}

bool Plane::operator != (const Plane& Other ) const
{
	return ( Other.Normal() != Normal() || Other.D() != D() );
}

Plane Plane::operator -() const
{
	return Plane( -Normal_, D_ );
}

