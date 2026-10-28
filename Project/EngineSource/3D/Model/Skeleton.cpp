#include "pch.h"
#include "Skeleton.h"
#include <cassert>
#include <algorithm>
#include "MyMath.h"
using namespace GameEngine;

void Skeleton::Create(ID3D12GraphicsCommandList4* cmdList, const SkeletonData& skeletonData,const ModelData& modelData) {
	assert(!skeletonData.joints.empty() && "Skeleton joints are empty!");
	assert(!modelData.meshes.empty() && "Model has no meshes!");
	// メッシュ単位に分離されたskinClusterDataを前提とする
	assert(modelData.skinClusterData.size() == modelData.meshes.size() && "skinClusterData must be split per mesh (see ModelLoader)");

	const uint32_t meshCount = static_cast<uint32_t>(modelData.meshes.size());

	// スケルトンデータを取得
	skeletonData_ = skeletonData;

	// 全メッシュ共通のpalette用のリソースを作成
	skinCluster_.wellBuffer.Create(static_cast<uint32_t>(skeletonData_.joints.size()));
	auto* mappedPalette = skinCluster_.wellBuffer.GetData();
	skinCluster_.mappedPalette = { mappedPalette, skeletonData_.joints.size() };

	// 全メッシュ共通のinverseBindPoseMatrixを格納する場所を作成して、単位行列で埋める
	skinCluster_.inverseBindPoseMatrices.resize(skeletonData_.joints.size());
	std::generate(skinCluster_.inverseBindPoseMatrices.begin(), skinCluster_.inverseBindPoseMatrices.end(), Matrix4x4::MakeIdentity);

	// メッシュ数分のリソースを確保
	outputVertexBuffers_.resize(meshCount);
	constBuffers_.resize(meshCount);
	influenceBuffers_.resize(meshCount);
	mappedInfluences_.resize(meshCount);
	verticesNums_.resize(meshCount);

	// メッシュsごとにスキニング用リソースを作成する
	for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex) {
		const auto& meshVertices = modelData.meshes[meshIndex].vertices;
		assert(!meshVertices.empty() && "Model mesh has no vertices!");

		// アウトプット用の頂点リソースを作成
		outputVertexBuffers_[meshIndex].Create(cmdList, meshVertices);

		// 頂点数を取得
		verticesNums_[meshIndex] = static_cast<uint32_t>(meshVertices.size());

		// 定数バッファを生成
		constBuffers_[meshIndex].Create();
		constBuffers_[meshIndex].GetData()->numVertices = static_cast<uint32_t>(meshVertices.size());

		// influence(頂点ごとのボーン影響度)を0埋めで確保
		std::vector<VertexInfluence> influence;
		influence.resize(meshVertices.size());
		for (auto& inf : influence) {
			inf.weights.fill(0.0f);
		}
		influenceBuffers_[meshIndex].Create(influence);
		// spanを使ってアクセスするようにする
		VertexInfluence* mappedInfluence = influenceBuffers_[meshIndex].GetVertexData();
		mappedInfluences_[meshIndex] = { mappedInfluence, meshVertices.size() };

		// このメッシュに対応するボーンウェイト情報を解析
		const auto& meshSkinClusterData = modelData.skinClusterData[meshIndex];
		for (const auto& jointWeight : meshSkinClusterData) {
			// jointWeight.firstはjoint名なので、skeletonに対象となるjointが含まれているか判断
			auto it = skeletonData_.jointMap.find(jointWeight.first);
			if (it == skeletonData_.jointMap.end()) { // 存在しない場合は次に回す
				continue;
			}

			// (*it).secondにはjointのindexが入っているので、該当のindexのinverseBindPoseMatrixを代入
			skinCluster_.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;

			for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
				auto& currentInfluence = mappedInfluences_[meshIndex][vertexWeight.vertexIndex];
				for (uint32_t index = 0; index < kNumMaxInfluence; ++index) { // 空いているところに入れる
					if (currentInfluence.weights[index] == 0.0f) { // weight==0が空いている状態なので、その場所にweightとjointのindexを代入
						currentInfluence.weights[index] = vertexWeight.weight;
						currentInfluence.jointIndices[index] = (*it).second;
						break;
					}
				}
			}
		}
	}
}