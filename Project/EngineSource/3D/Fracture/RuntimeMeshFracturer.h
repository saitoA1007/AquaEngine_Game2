#pragma once
#include <vector>
#include "Vector3.h"
#include "VertexData.h"

namespace GameEngine {

	// 平面クリッピングの結果
	struct ClipResult {
		std::vector<VertexData> frontVerts;
		std::vector<VertexData> backVerts;
		std::vector<uint32_t> frontIndices;
		std::vector<uint32_t> backIndices;
	};

	/// <summary>
	/// メッシュに対する計算幾何操作を行うアルゴリズム
	/// </summary>
	namespace RuntimeMeshFracturer {

		// 衝撃点周りをクレーター状に削ったうえでボロノイ分割し、断片群を返す。クレーターでメッシュ全体が消えた場合は元のメッシュにフォールバックする
		std::vector<Fragment> Fracture(
			const Fragment& source, const Vector3& impactPos,
			float craterRadius, int numSites, int planeCount,
			size_t minTriangleCount);
	}
}
