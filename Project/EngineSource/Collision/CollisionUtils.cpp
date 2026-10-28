#include "pch.h"
#include "CollisionUtils.h"

#include "MyMath.h"
#include <cmath>
#include <algorithm>

using namespace GameEngine;

// 衝突フラグしか値を返さないものが存在している
// IsSpherePlaneCollision, IsSegmentPlaneCollision, IsSegmentTriangleCollision, 
// IsAABBSegmentCollision,IsOBBSegmentCollision,

CollisionResult GameEngine::IsSpheresCollision(const Sphere& s1, const Sphere& s2) {
	CollisionResult result;

	Vector3 diff = s2.center - s1.center;
	float distance = diff.Length();

	// 半径の合計より短ければ衝突
	if (distance < s1.radius + s2.radius) {
		result.isHit = true;
		// 接触法線
		if (distance > 0.0f) {
			result.contactNormal = diff / distance;
		} else {
			// 完全に重なっている場合
			result.contactNormal = { 1.0f, 0.0f, 0.0f };
		}

		// 接触点
		result.contactPosition = s1.center + result.contactNormal * s1.radius;

		// 侵入深度
		result.penetrationDepth = s1.radius + s2.radius - distance;
	} 
	return result;
}

CollisionResult GameEngine::IsSpherePlaneCollision(const Sphere& sphere, const Plane& plane) {
	CollisionResult result;

	// 球の半径より短ければ衝突
	if (std::fabs(plane.normal.Dot(sphere.center) - plane.distance) <= sphere.radius) {
		result.isHit = true;
	}

	return result;
}

CollisionResult GameEngine::IsSegmentPlaneCollision(const Segment& segment, const Plane& plane) {
	CollisionResult result;

	// 垂直判定を行うために、法線と線の内積を求める
	float dot = plane.normal.Dot(segment.diff);

	// 垂直の時は衝突していないのでfalseを返す
	if (dot == 0.0f) {
		return result;
	}

	// tを求める
	float t = (plane.distance - plane.normal.Dot(segment.origin)) / dot;

	if (t >= 0.0f && t <= 1.0f) {
		result.isHit = true;
	} 

	return result;
}

CollisionResult GameEngine::IsSegmentTriangleCollision(const Triangle& triangle, const Segment& segment) {
	CollisionResult result;

	// 三角形の3つの頂点を使って平面を求める
	Plane plane;
	plane.normal = Vector3::Cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[1]);
	plane.distance = plane.normal.Dot(triangle.vertices[0]);
	// 法線を正規化
	plane.normal = Vector3::Normalize(plane.normal);
	// 垂直判定を行うために、法線と線の内積を求める
	float dot = plane.normal.Dot(segment.diff);

	// tを求める
	float t = (plane.distance - plane.normal.Dot(segment.origin)) / dot;
	// 衝突点pを求める
	Vector3 p = Vector3(segment.origin) + Vector3(segment.diff.x * t, segment.diff.y * t, segment.diff.z * t);

	// 各辺を結んだベクトル
	Vector3 v01 = triangle.vertices[1] - triangle.vertices[0];
	Vector3 v1p = p - triangle.vertices[1];
	Vector3 v12 = triangle.vertices[2] - triangle.vertices[1];
	Vector3 v2p = p - triangle.vertices[2];
	Vector3 v20 = triangle.vertices[0] - triangle.vertices[2];
	Vector3 v0p = p - triangle.vertices[0];

	// 各辺を結んだベクトルと、頂点と衝突点pを結んだベクトルのクロス積を取る
	Vector3 cross01 = v01.Cross(v1p);
	Vector3 cross12 = v12.Cross(v2p);
	Vector3 cross20 = v20.Cross(v0p);

	// すべての小三角形のクロス積と法線が同じ方向を向いていたら衝突
	if (cross01.Dot(plane.normal) >= 0.0f &&
		cross12.Dot(plane.normal) >= 0.0f &&
		cross20.Dot(plane.normal) >= 0.0f) {
		result.isHit = true;
	}

	return result;
}

CollisionResult GameEngine::IsAABBCollision(const AABB& aabb1, const AABB& aabb2) {
	CollisionResult result;

	// 各軸での重なりをチェック
	bool overlapX = aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x;
	bool overlapY = aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y;
	bool overlapZ = aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z;

	if (overlapX && overlapY && overlapZ) {
		result.isHit = true;

		// 各軸の重なり量を計算
		float overlapXAmount = std::min(aabb1.max.x - aabb2.min.x, aabb2.max.x - aabb1.min.x);
		float overlapYAmount = std::min(aabb1.max.y - aabb2.min.y, aabb2.max.y - aabb1.min.y);
		float overlapZAmount = std::min(aabb1.max.z - aabb2.min.z, aabb2.max.z - aabb1.min.z);

		// 最も重なりが少ない軸を見つける
		if (overlapXAmount < overlapYAmount && overlapXAmount < overlapZAmount) {
			// X軸が最小の重なり
			result.penetrationDepth = overlapXAmount;
			result.contactNormal = { (aabb1.max.x + aabb1.min.x) > (aabb2.max.x + aabb2.min.x) ? 1.0f : -1.0f, 0.0f, 0.0f };
		} else if (overlapYAmount < overlapZAmount) {
			// Y軸が最小の重なり
			result.penetrationDepth = overlapYAmount;
			result.contactNormal = { 0.0f, (aabb1.max.y + aabb1.min.y) > (aabb2.max.y + aabb2.min.y) ? 1.0f : -1.0f, 0.0f };
		} else {
			// Z軸が最小の重なり
			result.penetrationDepth = overlapZAmount;
			result.contactNormal = { 0.0f, 0.0f, (aabb1.max.z + aabb1.min.z) > (aabb2.max.z + aabb2.min.z) ? 1.0f : -1.0f };
		}

		// 接触点
		Vector3 contactMin = {
			std::max(aabb1.min.x, aabb2.min.x),
			std::max(aabb1.min.y, aabb2.min.y),
			std::max(aabb1.min.z, aabb2.min.z)
		};
		Vector3 contactMax = {
			std::min(aabb1.max.x, aabb2.max.x),
			std::min(aabb1.max.y, aabb2.max.y),
			std::min(aabb1.max.z, aabb2.max.z)
		};
		result.contactPosition = (contactMin + contactMax) * 0.5f;
	}

	return result;
}

CollisionResult GameEngine::IsAABBSphereCollision(const AABB& aabb, const Sphere& sphere) {
	CollisionResult result;

	// 最近接点を求める
	Vector3 closestPoint = {
		std::clamp(sphere.center.x,aabb.min.x,aabb.max.x),
		std::clamp(sphere.center.y,aabb.min.y,aabb.max.y),
		std::clamp(sphere.center.z,aabb.min.z,aabb.max.z)
	};

	Vector3 diff =  sphere.center - closestPoint;
	float distance = diff.Length();

	// 距離が半径よりも小さければ衝突
	if (distance <= sphere.radius) {
		result.isHit = true;

		// 接触点
		result.contactPosition = closestPoint;

		// 接触法線
		if (distance > 0.0f) {	
			result.contactNormal = diff / distance;
			result.penetrationDepth = sphere.radius - distance;
		} else {
			result.contactNormal = {0.0f,0.0f,1.0f};
			result.penetrationDepth = sphere.radius;
		}
	} 
	return result;
}

CollisionResult GameEngine::IsAABBSegmentCollision(const AABB& aabb, const Segment& segment) {
	CollisionResult result;

	float tNear = -std::numeric_limits<float>::infinity();
	float tFar = std::numeric_limits<float>::infinity();

	// 各軸について計算
	for (int i = 0; i < 3; ++i) {
		float origin = (i == 0) ? segment.origin.x : (i == 1) ? segment.origin.y : segment.origin.z;
		float diff = (i == 0) ? segment.diff.x : (i == 1) ? segment.diff.y : segment.diff.z;
		float min = (i == 0) ? aabb.min.x : (i == 1) ? aabb.min.y : aabb.min.z;
		float max = (i == 0) ? aabb.max.x : (i == 1) ? aabb.max.y : aabb.max.z;

		if (std::abs(diff) < 1e-6f) {
			if (origin < min || origin > max) { return result; }
		} else {
			float t1 = (min - origin) / diff;
			float t2 = (max - origin) / diff;

			float tMinAxis = std::min(t1, t2);
			float tMaxAxis = std::max(t1, t2);

			tNear = std::max(tNear, tMinAxis);
			tFar = std::min(tFar, tMaxAxis);
		}
	}

	// 衝突判定
	if (tNear <= tFar && tFar >= 0.0f && tNear <= 1.0f) {
		result.isHit = true;

		float t = std::max(0.0f, tNear);
		result.contactPosition.x = segment.origin.x + t * segment.diff.x;
		result.contactPosition.y = segment.origin.y + t * segment.diff.y;
		result.contactPosition.z = segment.origin.z + t * segment.diff.z;
	}

	return result;
}

CollisionResult GameEngine::IsOBBSphereCollision(const OBB& obb, const Sphere& sphere) {
	CollisionResult result;

	// 球からobbの中心へのベクトル
	Vector3 v = sphere.center - obb.center;

	// ベクトルをOBBのローカル座標系に変換
	Vector3 centerInOBBLocalSpace = {
		v.Dot(obb.orientations[0]),
		v.Dot(obb.orientations[1]),
		v.Dot(obb.orientations[2])
	};

	// obbのローカル空間の球の位置を求める
	Sphere sphereOBBLocal;
	sphereOBBLocal.center = centerInOBBLocalSpace;
	sphereOBBLocal.radius = sphere.radius;

	// obbのローカル空間の大きさを求める
	AABB aabbLocal;
	aabbLocal.min = obb.size * -1.0f;
	aabbLocal.max = obb.size;

	// ローカル空間での最近接点を求める
	Vector3 closestPointLocal = {
		std::clamp(centerInOBBLocalSpace.x, aabbLocal.min.x, aabbLocal.max.x),
		std::clamp(centerInOBBLocalSpace.y, aabbLocal.min.y, aabbLocal.max.y),
		std::clamp(centerInOBBLocalSpace.z, aabbLocal.min.z, aabbLocal.max.z)
	};

	// ローカル空間での距離を計算
	Vector3 diffLocal = centerInOBBLocalSpace - closestPointLocal;
	float distanceSquared = diffLocal.Dot(diffLocal);
	float radiusSquared = sphere.radius * sphere.radius;

	if (distanceSquared <= radiusSquared) {
		result.isHit = true;

		float distance = std::sqrt(distanceSquared);

		// 最近接点をワールド座標に変換
		result.contactPosition = obb.center +
			obb.orientations[0] * closestPointLocal.x +
			obb.orientations[1] * closestPointLocal.y +
			obb.orientations[2] * closestPointLocal.z;

		if (distance > 0.0f) {
			// ローカル空間での法線を計算
			Vector3 normalLocal = diffLocal / distance;

			// 法線をワールド座標に変換
			result.contactNormal =
				obb.orientations[0] * normalLocal.x +
				obb.orientations[1] * normalLocal.y +
				obb.orientations[2] * normalLocal.z;

			result.contactNormal.Normalize();
			result.penetrationDepth = sphere.radius - distance;
		} else {
			result.contactNormal = {0.0f,0.0f,1.0f};
			result.penetrationDepth = sphere.radius;
		}
	}
	return result;
}

CollisionResult GameEngine::IsOBBSegmentCollision(const OBB& obb, const Segment& segment) {
	CollisionResult result;

	// ワールド座標での線の終点を求める
	Vector3 segmentEnd = segment.origin + segment.diff;

	// 線の始点をOBBのローカル座標系に変換
	Vector3 vOrigin = segment.origin - obb.center;
	Vector3 localOrigin = {
		vOrigin.Dot(obb.orientations[0]),
		vOrigin.Dot(obb.orientations[1]),
		vOrigin.Dot(obb.orientations[2])
	};

	// 線の終点をOBBのローカル座標系に変換
	Vector3 vEnd = segmentEnd - obb.center;
	Vector3 localEnd = {
		vEnd.Dot(obb.orientations[0]),
		vEnd.Dot(obb.orientations[1]),
		vEnd.Dot(obb.orientations[2])
	};

	// ローカル空間での線を求める
	Segment segmentLocal;
	segmentLocal.origin = localOrigin;
	segmentLocal.diff = localEnd - localOrigin;

	// obbのローカル空間の大きさを求める
	AABB aabbOBBLocal;
	aabbOBBLocal.min = obb.size * -1.0f;
	aabbOBBLocal.max = obb.size;

	// ローカル空間での衝突判定の結果を返す
	return IsAABBSegmentCollision(aabbOBBLocal, segmentLocal);
}