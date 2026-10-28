#pragma once
#include "Geometry.h"
#include "CollisionResult.h"

namespace GameEngine {

	// 球同士の衝突判定
	CollisionResult IsSpheresCollision(const Sphere& s1, const Sphere& s2);

	// 平面と球の衝突判定
	CollisionResult IsSpherePlaneCollision(const Sphere& sphere, const Plane& plane);

	// 線と平面の衝突判定
	CollisionResult IsSegmentPlaneCollision(const Segment& segment, const Plane& plane);

	// 線と三角形の衝突判定
	CollisionResult IsSegmentTriangleCollision(const Triangle& triangle, const Segment& segment);

	// aabb同士の衝突判定
	CollisionResult IsAABBCollision(const AABB& aabb1, const AABB& aabb2);

	// aabbと球の衝突判定
	CollisionResult IsAABBSphereCollision(const AABB& aabb, const Sphere& sphere);

	// aabbと線の衝突判定
	CollisionResult IsAABBSegmentCollision(const AABB& aabb, const Segment& segment);

	// obbと球の衝突判定
	CollisionResult IsOBBSphereCollision(const OBB& obb, const Sphere& sphere);

	// obbと線の衝突判定
	CollisionResult IsOBBSegmentCollision(const OBB& obb, const Segment& segment);
}