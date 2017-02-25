#pragma once

#include "math/dll.h"
#include "math/mat44.h"
#include "math/plane.h"

namespace Math
{
	/**
	 * Axis aligned bounding box.
	 */
	class MATH_DLL AABB final
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

		void Minimum(const Vec3& Min) { Min_ = Min; }
		void Maximum(const Vec3& Max) { Max_ = Max; }
		const Vec3& Minimum() const { return Min_; }
		const Vec3& Maximum() const { return Max_; }

		f32 Width() const { return Max_.x - Min_.x; }
		f32 Height() const { return Max_.y - Min_.y; }
		f32 Depth() const { return Max_.z - Min_.z; }
		f32 Volume() const { return (Width() * Height() * Depth()); }

		Vec3 Corner(u32 i) const;
		Plane FacePlane(u32 i) const;
		Vec3 FaceCentre(u32 i) const;
		Vec3 Centre() const { return ((Min_ + Max_) * 0.5f); }
		Vec3 Dimensions() const { return (Max_ - Min_); }
		f32 Diameter() const { return ((Max_ - Min_).Magnitude()); }

		bool IsEmpty() const { return ((Min_.x > Max_.x) || (Min_.y > Max_.y) || (Min_.z > Max_.z)); }

		// Construction
		void Empty();
		void ExpandBy(const Vec3& Point);
		void ExpandBy(const AABB& AABB);

		// Intersection
		bool LineIntersect(
		    const Vec3& Start, const Vec3& End, Vec3* pIntersectionPoint, Vec3* pIntersectionNormal) const;
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
} // namespace Math
