#include "math/aabb.h"
#include "math/float.h"
#include "core/debug.h"
#include "core/misc.h"

namespace Math
{
	AABB::AABB() { Empty(); }

	AABB::AABB(const AABB& Other)
	    : Min_(Other.Min_)
	    , Max_(Other.Max_)
	{
	}

	AABB::AABB(const Vec3& Min, const Vec3& Max)
	    : Min_(Min)
	    , Max_(Max)
	{
	}

	AABB::~AABB() {}

	Plane AABB::FacePlane(u32 i) const
	{
		Plane Plane;

		switch(i)
		{
		case FRONT:
			Plane.FromPoints(Corner(LTF), Corner(LBF), Corner(RBF));
			break;
		case BACK:
			Plane.FromPoints(Corner(RTB), Corner(RBB), Corner(LBB));
			break;
		case TOP:
			Plane.FromPoints(Corner(LTF), Corner(RTF), Corner(RTB));
			break;
		case BOTTOM:
			Plane.FromPoints(Corner(LBB), Corner(RBB), Corner(RBF));
			break;
		case LEFT:
			Plane.FromPoints(Corner(LBB), Corner(LBF), Corner(LTF));
			break;
		case RIGHT:
			Plane.FromPoints(Corner(RTB), Corner(RTF), Corner(RBF));
			break;
		default:
			DBG_BREAK;
		}

		return Plane;
	}

	Vec3 AABB::FaceCentre(u32 i) const
	{
		Vec3 FaceCentre = Centre();
		switch(i)
		{
		case FRONT:
		case BACK:
			FaceCentre += FacePlane(i).Normal() * (Depth() * 0.5f);
			break;
		case TOP:
		case BOTTOM:
			FaceCentre += FacePlane(i).Normal() * (Height() * 0.5f);
			break;
		case LEFT:
		case RIGHT:
			FaceCentre += FacePlane(i).Normal() * (Width() * 0.5f);
			break;
		default:
			DBG_BREAK;
		}

		return FaceCentre;
	}

	bool AABB::LineIntersect(
	    const Vec3& Start, const Vec3& End, Vec3* pIntersectionPoint, Vec3* pIntersectionNormal) const
	{
		// Planes. Screw it.
		// Totally inoptimal.
		Plane Planes[6];
		Vec3 Intersects[6];
		f32 Distance;
		for(u32 i = 0; i < 6; ++i)
		{
			Planes[i] = FacePlane(i);
			if(!Planes[i].LineIntersection(Start, End, Distance, Intersects[i]))
			{
				Intersects[i] = Vec3(1e24f, 1e24f, 1e24f);
			}
		}

		// Reject classified and find nearest.
		f32 Nearest = 1e24f;
		u32 iNearest = 0xffffffff;

		for(u32 i = 0; i < 6; ++i)
		{
			// For every point...
			// ...check against planes.
			bool Valid = true;
			for(u32 j = 0; j < 6; ++j)
			{
				if(Planes[j].Classify(Intersects[i]) == Plane::FRONT)
				{
					Valid = false;
					break;
				}
			}

			// If its valid, check distance.
			if(Valid)
			{
				Distance = (Start - Intersects[i]).MagnitudeSquared();

				if(Distance < Nearest)
				{
					Nearest = Distance;
					iNearest = i;
				}
			}
		}

		//
		if(iNearest != 0xffffffff)
		{
			if(pIntersectionPoint != NULL)
			{
				*pIntersectionPoint = Intersects[iNearest];
			}

			if(pIntersectionNormal != NULL)
			{
				*pIntersectionNormal = Planes[iNearest].Normal();
			}

			return true;
		}
		return false;
	}

	bool AABB::BoxIntersect(const AABB& Box, AABB* pIntersectionBox) const
	{
		// Check for no overlap.
		if((Min_.x > Box.Max_.x) || (Max_.x < Box.Min_.x) || (Min_.y > Box.Max_.y) || (Max_.y < Box.Min_.y) ||
		    (Min_.z > Box.Max_.z) || (Max_.z < Box.Min_.z))
		{
			return false;
		}

		// Overlap, compute AABB of intersection.
		if(pIntersectionBox != NULL)
		{
			pIntersectionBox->Min_.x = Core::Max(Min_.x, Box.Min_.x);
			pIntersectionBox->Max_.x = Core::Min(Max_.x, Box.Max_.x);
			pIntersectionBox->Min_.y = Core::Max(Min_.y, Box.Min_.y);
			pIntersectionBox->Max_.y = Core::Min(Max_.y, Box.Max_.y);
			pIntersectionBox->Min_.z = Core::Max(Min_.z, Box.Min_.z);
			pIntersectionBox->Max_.z = Core::Min(Max_.z, Box.Max_.z);
		}

		return true;
	}

	AABB::eClassify AABB::Classify(const Vec3& Point) const
	{
		if((Point.x >= Min_.x && Point.x <= Max_.x) && (Point.y >= Min_.y && Point.y <= Max_.y) &&
		    (Point.z >= Min_.z && Point.z <= Max_.z))
		{
			return INSIDE;
		}

		return OUTSIDE;
	}

	AABB::eClassify AABB::Classify(const AABB& AABB) const
	{
		u32 PointsInside = 0;
		for(u32 i = 0; i < 8; ++i)
		{
			Vec3 Point = AABB.Corner(i);
			if((Point.x >= Min_.x && Point.x <= Max_.x) && (Point.y >= Min_.y && Point.y <= Max_.y) &&
			    (Point.z >= Min_.z && Point.z <= Max_.z))
			{
				PointsInside++;
			}
		}

		if(PointsInside == 8)
		{
			return INSIDE;
		}

		if(PointsInside > 0)
		{
			return SPANNING;
		}

		return OUTSIDE;
	}

	AABB AABB::Transform(const Mat44& Transform) const
	{
		AABB NewAABB;

		// Add transformed corners.
		NewAABB.ExpandBy(Corner(0) * Transform);
		NewAABB.ExpandBy(Corner(1) * Transform);
		NewAABB.ExpandBy(Corner(2) * Transform);
		NewAABB.ExpandBy(Corner(3) * Transform);
		NewAABB.ExpandBy(Corner(4) * Transform);
		NewAABB.ExpandBy(Corner(5) * Transform);
		NewAABB.ExpandBy(Corner(6) * Transform);
		NewAABB.ExpandBy(Corner(7) * Transform);

		DBG_ASSERT(!NewAABB.IsEmpty());
		return NewAABB;
	}

	Vec3 AABB::Corner(u32 i) const
	{
		return Vec3((i & 1) ? Min_.x : Max_.x, (i & 2) ? Min_.y : Max_.y, (i & 4) ? Min_.z : Max_.z);
	}

	void AABB::Empty()
	{
		Min_ = Vec3(1e24f, 1e24f, 1e24f);
		Max_ = Vec3(-1e24f, -1e24f, -1e24f);
	}


	void AABB::ExpandBy(const Vec3& Point)
	{
		Min_.x = Core::Min(Min_.x, Point.x);
		Min_.y = Core::Min(Min_.y, Point.y);
		Min_.z = Core::Min(Min_.z, Point.z);

		Max_.x = Core::Max(Max_.x, Point.x);
		Max_.y = Core::Max(Max_.y, Point.y);
		Max_.z = Core::Max(Max_.z, Point.z);
	}

	void AABB::ExpandBy(const AABB& AABB)
	{
		DBG_ASSERT(!AABB.IsEmpty());
		Min_.x = Core::Min(Min_.x, AABB.Min_.x);
		Min_.y = Core::Min(Min_.y, AABB.Min_.y);
		Min_.z = Core::Min(Min_.z, AABB.Min_.z);

		Max_.x = Core::Max(Max_.x, AABB.Max_.x);
		Max_.y = Core::Max(Max_.y, AABB.Max_.y);
		Max_.z = Core::Max(Max_.z, AABB.Max_.z);
	}

} // namespace Math