#pragma once
#include <vector>
#include <memory>
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Vector3.h"
#include "Vector4.h"

namespace GameEngine {

	// 前方宣言
	class Model;
	class RenderQueue;

	/// <summary>
	/// Sphereモデルをグリッド状に並べ、metallic、 roughness の違いを比較するデモ
	///   X軸 : roughness  (右にいく程粗い)
	///   Y軸 : metallic   (上にいく程金属)
	/// </summary>
	class PBRDemo : public GameEngine::IGameObject {
	public:
		PBRDemo(Model* sphereModel, uint32_t metallicSteps = 5, uint32_t roughnessSteps = 5);
		~PBRDemo() = default;

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

		// デバックの更新
		void DebugUpdate() override;

	private:
		// 球体1つ分
		std::vector<std::unique_ptr<ModelComponent>> spheres_;

		Model* model_ = nullptr;

		// グリッドサイズ
		uint32_t rows_ = 0;
		uint32_t cols_ = 0;

		// 調整用パラメータ
		Vector4 baseColor_ = { 0.95f, 0.64f, 0.54f, 1.0f };
		float spacing_ = 2.5f;
		float sphereScale_ = 1.0f;
		Vector3 origin_ = { 16.0f, 0.0f, 0.0f };

		// roughness=0はGGXが破綻するので下限を設ける
		float minRoughness_ = 0.05f;
		float maxRoughness_ = 1.0f;

		// 配置の更新フラグ
		bool layoutDirty_ = true;

	private:
		// グリッドの配置を再計算
		void ApplyLayout();

		// 1つ分のmetallic / roughnessを取得
		float GetMetallic(uint32_t row) const;
		float GetRoughness(uint32_t col) const;

	};
}
