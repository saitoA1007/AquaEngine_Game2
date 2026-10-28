#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "Collider.h"
#include "ParticleBehavior.h"

class WindAttack : public GameEngine::IGameObject {
public:
	WindAttack(GameEngine::Model* model, GameEngine::ParticleBehavior* windParticle);
	~WindAttack() = default;

	// 初期化
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 開始するための状態を設定
	void Start(Vector3 pos, Vector3 startDir, Vector3 endDir, float maxTime) {
		modelComponent_.worldTransform_.transform_.translate = pos;
		startDir_ = startDir;
		endDir_ = endDir;
		maxTime_ = maxTime;
		timer_ = 0.0f;
		isActive_ = true;
		// 当たり判定を有効化
		collider_.SetActive(true);

		// 風の演出を設定
		if (windParticle_ != nullptr) {
			windParticle_->SetEmitterPos(modelComponent_.worldTransform_.transform_.translate);
			windParticle_->SetDirection(startDir_);
			windParticle_->SetIsLoop(true);
		}
	}

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// モデル
	GameEngine::ModelComponent modelComponent_;

	// obbの当たり判定
	GameEngine::OBBCollider collider_;

	float timer_ = 0.0f;

	float maxTime_ = 2.0f;

	Vector3 startDir_ = { 0.0f,0.0f,-1.0f };
	Vector3 endDir_ = { 0.0f,0.0f,-1.0f };

	// 風の軌跡を表すパーティクル
	GameEngine::ParticleBehavior* windParticle_ = nullptr;

private:

	// 当たり判定
	Vector3 colliderSize_ = {1.0f,1.0f,16.0f};
	Vector3 colliderAnchor_ = { 0.5f,0.5f,0.0f };

private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);
};