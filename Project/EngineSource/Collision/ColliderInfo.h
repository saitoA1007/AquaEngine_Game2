#pragma once
#include <variant>
#include "Geometry.h"

namespace GameEngine {

	// 形状
	enum ShapeType {
		kSphere,
		kAABB,
		kOBB,
		kSegment,

		kMaxCount
	};
	inline constexpr const char* ShapeTypeNames[] = {
		"Sphere", "AABB", "OBB", "Segment"
	};

	struct ColliderShapeData {
		ShapeType type = ShapeType::kSphere;

		// SphereHemisphere
		float radius = 1.0f;

		// アンカーポイント
		Vector3 anchorPoint = {0.5f,0.5f,0.5f};

		// 回転
		Vector3 rotate = {0.0f, 0.0f, 0.0f};

		// Box
		Vector3 boxSize = { 1.0f, 1.0f, 1.0f };
	};

	// 当たり判定の形状
	struct CollisionData {
		// 当たり判定の属性
		std::variant<Sphere, AABB, OBB, Segment> data;
		// 形状タイプ
		ShapeType shapeType;

		// 形状を取得する
		template<typename T>
		const T* Get() const { return std::get_if<T>(&data); }
		template<typename T>
		T* Get() { return std::get_if<T>(&data); }
	};
}