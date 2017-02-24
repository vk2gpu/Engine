#pragma once

#include "math/dll.h"
#include "math/mat44.h"
#include "math/plane.h"

//////////////////////////////////////////////////////////////////////////
// Definition
class MATH_DLL AABB
{
public:
	enum eClassify
	{
		INSIDE = 0,
		OUTSIDE,
		SPANNING
	};

	enum eCorners
	{
		LBB = 0,
		RBB,
		LTB,
		RTB,
		LBF,
		RBF,
		LTF,
		RTF,
	};

	enum eFaces
	{
		LEFT = 0,
		RIGHT,
		TOP,
		BOTTOM,
		FRONT,
		BACK,
	};


public:
	AABB();
	AABB(const AABB& Other);
	AABB(const Vec3& Min, const Vec3& Max);
	~AABB();

	void Minimum(const Vec3& Min);
	void Maximum(const Vec3& Max);

	const Vec3& Minimum() const;
	const Vec3& Maximum() const;

	f32 Width() const;
	f32 Height() const;
	f32 Depth() const;
	f32 Volume() const;

	Vec3 Corner(u32 i) const;
	Plane FacePlane(u32 i) const;
	Vec3 FaceCentre(u32 i) const;
	Vec3 Centre() const;
	Vec3 Dimensions() const;
	f32 Diameter() const;

	bool IsEmpty() const;

	// Construction
	void Empty();
	void ExpandBy(const Vec3& Point);
	void ExpandBy(const AABB& AABB);

	// Intersection
	bool LineIntersect(const Vec3& Start, const Vec3& End, Vec3* pIntersectionPoint, Vec3* pIntersectionNormal) const;
	bool BoxIntersect(const AABB& Box, AABB* pIntersectionBox) const;

	// Classification
	eClassify Classify(const Vec3& Point) const;
	eClassify Classify(const AABB& AABB) const;

	// Transform
	AABB Transform(const Mat44& Transform) const;

private:
	Vec3 Min_;
	Vec3 Max_;
};
