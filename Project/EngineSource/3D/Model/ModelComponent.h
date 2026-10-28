#pragma once
#include <vector>
#include "DrawRequest.h"
#include "WorldTransform.h"
#include "Material.h"
#include "RefBuffer.h"

namespace GameEngine {

	// 前方宣言
	class RenderQueue;

	/// <summary>
	/// モデルのデータから加工をおこなう
	/// </summary>
	class ModelComponent {
	public:
		// モデルデータ
		ModelComponent(Model* model);

		// 更新処理
		void Update();

		// 描画処理
		void Draw(RenderQueue* renderQueue, const Draw3dType& drawType = Draw3dType::Default, const std::string& passName = "DefaultPass");

		// レイトレによる描画
		void DrawRaytracing(RenderQueue* renderQueue);

	public:

		// 参照値を設定
		void SetRefType(uint32_t type, uint32_t i = 0) {
			refBuffers_[i].SetType(type);
		}

		// 参照するマテリアルを設定
		void SetBufferMaterial(uint32_t type, uint32_t srvIndex, uint32_t i = 0) {
			refBuffers_[i].SetBufferMaterial(type, srvIndex);
		}

		// ヒットグループを設定
		void SetHitGroup(uint32_t hitGroupIndex, uint32_t i = 0) {
			refBuffers_[i].SetHitGroupIndex(hitGroupIndex);
		}

		// レイキャスト時のフィルタリング用マスクを設定
		void SetInstanceMask(uint32_t mask, uint32_t i = 0) {
			refBuffers_[i].SetInstanceMask(mask);
		}

	public:

		// モデルが持つワールド行列
		WorldTransform worldTransform_;

		// マテリアルデータ
		Material::MaterialData* materialData_ = nullptr;

	private:
		// モデルデータ
		Model* model_ = nullptr;

		// 標準のマテリアル
		std::vector<Material> defaultMaterials_;

		// 参照用
		std::vector<RefBuffer> refBuffers_;
	};
}

