#pragma once
#include <variant>
#include "Geometry.h"
#include "CollisionUtils.h"

namespace GameEngine {

	/// <summary>
	/// 当たり判定の組み合わせを設定する
	/// </summary>
	struct CollisionVisitor {

		// 球と球の当たり判定
		CollisionResult operator()(const Sphere& a, const Sphere& b) const {
			return IsSpheresCollision(a, b);
		}

		// AABBとAABBの当たり判定
		CollisionResult operator()(const AABB& a, const AABB& b) const {
			return IsAABBCollision(a, b);
		}

		// 球とAABBの当たり判定
		CollisionResult operator()(const Sphere& a, const AABB& b) const {
			CollisionResult result = IsAABBSphereCollision(b, a);
			// 法線の向きを反転
			if (result.isHit) {
				result.contactNormal = result.contactNormal * -1.0f;
			}
			return result;
		}

		CollisionResult operator()(const AABB& a, const Sphere& b) const {
			return IsAABBSphereCollision(a, b);
		}

		// AABBと線分の当たり判定
		CollisionResult operator()(const AABB& a, const Segment& b) const {
			return IsAABBSegmentCollision(a, b);
		}

		CollisionResult operator()(const Segment& a, const AABB& b) const {
			return IsAABBSegmentCollision(b, a);
		}

		// obbと球の当たり判定
		CollisionResult operator()(const OBB& a, const Sphere& b) const {
			return IsOBBSphereCollision(a, b);
		}

		CollisionResult operator()(const Sphere& b,const OBB& a) const {
			return IsOBBSphereCollision(a, b);
		}

		// obbと線の当たり判定
		CollisionResult operator()(const OBB& a, const Segment& b) const {
			return IsOBBSegmentCollision(a, b);
		}

		// 登録していない当たり判定は衝突しないようにする
		template <typename T, typename U>
		CollisionResult operator()(const T&, const U&) const {
			return CollisionResult{};
		}
	};
}