#pragma once
#include "VertexData.h"
#include "AnimationData.h"
#include "ConstantBuffer.h"

namespace GameEngine {

	// スキニング情報
	struct SkinningInformation
	{
		uint32_t numVertices; // 処理する頂点数
	};

	class Skeleton {
	public:
		
		/// <summary>
		/// ボーンデータを作成
		/// </summary>
		/// <param name="modelData"></param>
		void Create(ID3D12GraphicsCommandList4* cmdList, const SkeletonData& skeletonData,const ModelData& modelData);

		SkeletonData* GetSkeletonData() { return &skeletonData_; }

		SkinCluster* GetSkinCluster() { return &skinCluster_; }
		const SkinCluster* GetSkinClusterData() const { return &skinCluster_; }

		// スキニング後の出力頂点バッファを取得
		VertexBuffer<VertexData>* GetOutputVertexBuffer(uint32_t meshIndex) { return &outputVertexBuffers_[meshIndex]; }

		// スキニング計算用の定数バッファを取得
		ConstantBuffer<SkinningInformation>* GetConstantBuffer(uint32_t meshIndex) { return &constBuffers_[meshIndex]; }

		// 頂点ごとのボーン影響度バッファを取得
		VertexBuffer<VertexInfluence>* GetInfluenceBuffer(uint32_t meshIndex) { return &influenceBuffers_[meshIndex]; }
		const VertexBuffer<VertexInfluence>* GetInfluenceBufferData(uint32_t meshIndex) const { return &influenceBuffers_[meshIndex]; }

		// 処理対象の頂点数を取得
		uint32_t GetVerticesNum(uint32_t meshIndex) const { return verticesNums_[meshIndex]; }

		// スキニング後の出力頂点バッファのSRVインデックスを取得
		uint32_t GetOutputVertexBufferSrvIndex(uint32_t meshIndex) const { return outputVertexBuffers_[meshIndex].GetSrvIndex(); }

		// このスケルトンが対応しているメッシュの数を取得
		uint32_t GetMeshCount() const { return static_cast<uint32_t>(outputVertexBuffers_.size()); }

	private:
		SkeletonData skeletonData_;

		// 全メッシュ共通の骨行列パレット、バインドポーズ逆行列
		SkinCluster skinCluster_;

		// アウトプット用の頂点リソース
		std::vector<VertexBuffer<VertexData>> outputVertexBuffers_;
		// スキニング情報
		std::vector<ConstantBuffer<SkinningInformation>> constBuffers_;
		std::vector<VertexBuffer<VertexInfluence>> influenceBuffers_;
		std::vector<std::span<VertexInfluence>> mappedInfluences_;
		// 頂点数
		std::vector<uint32_t> verticesNums_;
	};
}