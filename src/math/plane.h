#pragma once

#include "math/dll.h"
#include "math/vec3.h"
#include "math/mat44.h"

namespace Math
{
	/**
	 * Plane
	 */
	class MATH_DLL Plane final
	{
	public:
		enum eClassify
		{
			FRONT = 0,
			BACK,
			COINCIDING,
			SPANNING
		};

	public:
		Plane() {}
		Plane(const Vec3& Normal, f32 D);
		Plane(f32 A, f32 B, f32 C, f32 D);
		Plane(const Vec3& V1, const Vec3& V2, const Vec3& V3);
		~Plane() {}

		void Normalise();
		void Transform(const Mat44& Transform);

		void FromPoints(const Vec3& V1, const Vec3& V2, const Vec3& V3);
		void FromPointNormal(const Vec3& Point, const Vec3& Normal);

		// Intersection
		bool LineIntersection(const Vec3& Point, const Vec3& Dir, f32& Distance) const;
		bool LineIntersection(const Vec3& A, const Vec3& B, f32& Distance, Vec3& Intersection) const;

		// Classification
		f32 Distance(const Vec3& P) const { return (Normal_.Dot(P)) - D_; }
		eClassify Classify(const Vec3& Point, f32 Radius = 1e-3f) const;

		// Operator
		bool operator==(const Plane& Other) const { return (Other.Normal() == Normal() && Other.D() == D()); }
		bool operator!=(const Plane& Other) const { return (Other.Normal() != Normal() || Other.D() != D()); }
		Plane operator-() const { return Plane(-Normal_, D_); }

		// Utility
		static bool Intersect(const Plane& A, const Plane& B, const Plane& C, Vec3& Point);

		const Vec3& Normal() const { return Normal_; }
		f32 D() const { return D_; };

	private:
		Vec3 Normal_;
		f32 D_;
	};

	inline Plane::Plane(const Vec3& Normal, f32 D)
	{
		Normal_ = Normal;
		D_ = D;
	}

	inline Plane::Plane(f32 A, f32 B, f32 C, f32 D)
	{
		Normal_ = Vec3(A, B, C);
		D_ = D;
	}

	inline Plane::Plane(const Vec3& V1, const Vec3& V2, const Vec3& V3)
	{ //
		FromPoints(V1, V2, V3);
	}
} // namespace Math