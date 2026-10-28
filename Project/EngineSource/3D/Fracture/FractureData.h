#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "Vector3.h"
#include "Geometry.h"

namespace GameEngine {

	/// <summary>
	/// 1つのバッファ内における、あるチャンクの頂点、インデックス範囲
	/// </summary>
	struct GeometryRange {
		uint32_t vertexOffset = 0;
		uint32_t vertexCount = 0;
		uint32_t indexOffset = 0;
		uint32_t indexCount = 0;
	};

	// 元のモデルの外側の表面を表すfacingChunkId
	inline constexpr uint32_t kFractureExteriorFace = 0xFFFFFFFF;

	/// <summary>
	/// 事前分割された破片1つ分のメタデータ
	/// </summary>
	struct FractureChunkInfo {
		// 同じ破壊オブジェクトに属するチャンクをまとめるグループ名
		std::string groupName;

		// グループ内でのチャンクID
		uint32_t chunkId = 0;

		// 隣接しているチャンクIDの一覧
		std::vector<uint32_t> neighborChunkIds;

		// 地面・壁などに固定されている破片か
		bool isAnchored = false;

		// 破片の重心
		Vector3 centroid{};

		// バウンディングボックス
		AABB aabb;

		// 三角形ごとに、その面が接している隣接チャンクのID。元の外側の表面ならkFractureExteriorFace
		std::vector<uint32_t> triangleFacingChunkIds;
	};
}