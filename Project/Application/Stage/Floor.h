#pragma once
#include "IGameObject.h"
#include "WorldTransform.h"
#include "Collider.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "IceMaterial.h"

class Floor : public GameEngine::IGameObject {
public:
	Floor(GameEngine::Model* model, uint32_t iceNormalGH, uint32_t iceHeightGH, uint32_t terrainGH, uint32_t terrainNormalGH);
	~Floor() = default;

	// 初期化
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:
	// ワールド行列を取得
	GameEngine::WorldTransform& GetWorldTransform() { return iceModelComponent_.worldTransform_; }

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 氷のモデル
	GameEngine::ModelComponent iceModelComponent_;

	// 氷のマテリアル
	GameEngine::IceMaterial iceMaterial_;

	// 地面のモデル
	GameEngine::ModelComponent terrainModelComponent_;

	// aabbの当たり判定
	GameEngine::AABBCollider collider_;

	// 当たり判定のサイズ
	Vector3 colliderSize_ = { 1.0f,1.0f,1.0f };
	// アンカーポイント
	Vector3 colliderAnchor_ = { 0.5f,0.5f,0.5f };

private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);
};