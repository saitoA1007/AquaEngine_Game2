#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Collider.h"
#include "IceMaterial.h"
#include "DestructibleObject.h"

// 前方宣言
namespace GameEngine {
	class DebugParameter;
}

class Wall : public GameEngine::IGameObject {
public:
	Wall(GameEngine::Model* model, GameEngine::Model* fractureModel, GameEngine::DebugParameter* parame);
	~Wall() = default;

	// 初期化
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

	void SetParameter(const Transform& transform);

private:

	// 最大hp
	int32_t maxHp_ = 3;

	// 復活するまでの時間
	float respawnTime_ = 3.0f;

	// 当たり判定の大きさ
	Vector3 colliderSize_ = { 10.0f,100.0f,0.5f };

	// 破壊された時に戻す割合
	float reassembleDestroyedRatio_ = 0.4f;

private:
	// パラメーター機能
	GameEngine::DebugParameter* parame_ = nullptr;

	// モデル
	//GameEngine::ModelComponent modelComponent_;

	// 氷のマテリアル
	GameEngine::IceMaterial iceMaterial_;

	// 下に存在する壁
	GameEngine::ModelComponent underWallModelComponent_;

	// タイマー
	float respawnTimer_ = 0.0f;

	// 現在のhp
	int32_t currentHp_ = 1;

	// obbの当たり判定
	GameEngine::OBBCollider collider_;

	// 氷の状態
	bool isBreakIce_ = false;

	// 破片のモデル
	GameEngine::DestructibleObject destructObject_;

private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);
};