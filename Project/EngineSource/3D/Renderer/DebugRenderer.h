#pragma once
#include <vector>
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "Geometry.h"
#include "VertexBuffer.h"

namespace GameEngine {

	// 頂点データ
	struct VertexPosColor {
		Vector4 pos;   // xyz座標
		Vector4 color; // RGBA
	};

	/// <summary>
	/// デバック用描画リソース
	/// </summary>
	class DebugRenderer final {
	public:

		// 線のデータ
		struct LineData {
			Vector3 start;
			Vector3 end;
			Vector4 color;
		};

	public:
		DebugRenderer();
		~DebugRenderer() = default;

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

		/// <summary>
		/// クリアする
		/// </summary>
		void Clear();

		/// <summary>
		/// 線を追加
		/// </summary>
		void AddLine(const Vector3& start, const Vector3& end, const Vector4& color = { 0,1,1,1 });

		/// <summary>
		/// AABBボックスを追加
		/// </summary>
		void AddAABB(const AABB& aabb, const Vector4& color = { 0,1,1,1 });

		/// <summary>
		/// AABBボックスを追加
		/// </summary>
		void AddBox(const Vector3& centerPos,const Vector3& size, const Vector4& color = { 0,1,1,1 });

		/// <summary>
		/// OBBボックスを追加
		/// </summary>
		void AddOBB(const OBB& obb, const Vector4& color = { 0,1,1,1 });

		/// <summary>
		/// 球を追加
		/// </summary>
		void AddSphere(const Sphere& sphere, const Vector4& color = { 0,1,1,1 }, int segments = 16);

		/// <summary>
		/// レイを追加
		/// </summary>
		void AddRay(const Segment segment, float length = 10.0f, const Vector4& color = { 0,1,1,1 });

		/// <summary>
		/// 2Dの円を追加
		/// </summary>
		void AddCircle(const Vector3& centerPos, const Vector3& normal, float radius, const Vector4& color = { 0,1,1,1 }, int segments = 16);

		/// <summary>
		/// デバッグ描画の有効化を設定
		/// </summary>
		void SetEnabled(bool enabled) { isEnabled_ = enabled; }
		bool IsEnabled() const { return isEnabled_; }

		// 頂点バッファビューを取得
		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBuffer_.GetView(); }
		const uint32_t& GetTotalVertices() const { return totalVertices_; }
	private:
		DebugRenderer(const DebugRenderer&) = delete;
		DebugRenderer& operator=(const DebugRenderer&) = delete;

		// 最大の頂点数
		uint32_t maxVertices_ = 10000;
		// 現在の頂点数
		uint32_t totalVertices_ = 0;
		bool isEnabled_ = true;

		// ラインの数を保持
		std::vector<LineData> lines_;
		
		// 頂点情報
		VertexBuffer<VertexPosColor> vertexBuffer_;
		VertexPosColor* vertexData_ = nullptr;
	};
}