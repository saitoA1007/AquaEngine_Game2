#pragma once
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexData.h"
#include "BLAS.h"

namespace GameEngine {

	class Mesh final {
	public:
		Mesh() = default;
		~Mesh() = default;

		/// <summary>
		/// 三角形の平面メッシュを作成
		/// </summary>
		void CreateTrianglePlaneMesh();

		/// <summary>
		/// グリッド平面のメッシュを作成
		/// </summary>
		/// <param name="size">x:横幅,y:縦幅</param>
		void CreateGridPlaneMesh(const Vector2& size);

		/// <summary>
		/// 平面のメッシュを作成
		/// </summary>
		/// <param name="size"></param>
		void CreatePlaneMesh(const Vector2& size);

		/// <summary>
		/// 球のメッシュを作成
		/// </summary>
		/// <param name="subdivision">分割数</param>
		void CreateSphereMesh(uint32_t subdivision);

		/// <summary>
		/// リングを作成する
		/// </summary>
		/// <param name="kRingDivide"></param>
		/// <param name="outerRadius"></param>
		/// <param name="innerRadius"></param>
		void CreateRingMesh(uint32_t ringDivide, float outerRadius, float innerRadius);

		/// <summary>
		/// 円柱を作成する
		/// </summary>
		/// <param name="cylinderDivide"></param>
		/// <param name="topRadius"></param>
		/// <param name="bottomRadius"></param>
		/// <param name="height"></param>
		void CreateCylinder(uint32_t cylinderDivide, float topRadius, float bottomRadius, float height);

		/// <summary>
		/// モデルデータを読み込んでメッシュを作成する
		/// </summary>
		/// <param name="modelData">読み込んだモデルデータ</param>
		void CreateModelMesh(ModelData modelData,const uint32_t& index);

		/// <summary>
		/// BLASを作成する
		/// </summary>
		/// <param name="cmdList"></param>
		void CreateBLAS(ID3D12GraphicsCommandList4* cmdList,const bool& isUpdate);

	public: // ゲッター

		// 頂点バッファビューを取得
		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBuffer_.GetView(); }
		// インデックスバッファビューを取得
		const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBuffer_.GetView(); }
		// 全ての頂点数を取得
		uint32_t GetTotalVertices() const { return vertexBuffer_.GetTotalVertices(); }
		// 全てのインデックス数を取得
		uint32_t GetTotalIndices() const { return indexBuffer_.GetTotalIndices(); }
		// メッシュに対応するマテリアル名を取得
		const std::string& GetMaterialName() const { return materialName_; }

		const VertexBuffer<VertexData>& GetVertexBuffer() const { return vertexBuffer_; }
		const IndexBuffer& GetIndexBuffer() const { return indexBuffer_; }

		// srvIndex
		uint32_t GetVertexBufferSrvIndex() const { return vertexBuffer_.GetSrvIndex(); }
		uint32_t GetIndexBufferSrvIndex() const { return indexBuffer_.GetSrvIndex(); }

		// blasを取得
		BLAS* GetBLAS() const { return blas_.get(); }

	private:
		VertexBuffer<VertexData> vertexBuffer_;
		IndexBuffer indexBuffer_;

		// blas
		std::unique_ptr<BLAS> blas_;

		std::string materialName_ = "default";
	};
}