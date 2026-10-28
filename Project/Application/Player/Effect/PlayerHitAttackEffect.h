#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"

class PlayerHitAttackEffect : public GameEngine::IGameObject {
public:
	PlayerHitAttackEffect(uint32_t texture, GameEngine::Model* planeModel);
	~PlayerHitAttackEffect() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	void Start(Vector3 pos, uint32_t level) {
		modelComponent_.worldTransform_.transform_.translate = pos;
		timer_ = 0.0f;
		isActive_ = true;

		// レベルによって大きさを変える
		float scale = 1.0f;
		switch (level)
		{
		case 1:
		default:
			scale = 5.0f;
			// 蒼
			modelComponent_.materialData_->color = Vector4(0.0f, 0.0f, 1.0f, 0.7f);
			break;

		case 2:
			scale = 10.0f;
			// 黄
			modelComponent_.materialData_->color = Vector4(1.0f, 1.0f, 0.0f, 0.7f);
			break;

		case 3:
			scale = 15.0f;
			// 赤
			modelComponent_.materialData_->color = Vector4(1.0f, 0.0f, 0.0f, 0.7f);
			break;
		}
		modelComponent_.worldTransform_.transform_.scale = Vector3(scale, scale, 1.0f);
	}

private:

	float maxTime_ = 1.0f;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// モデル
	GameEngine::ModelComponent modelComponent_;

	float timer_ = 0.0f;

	float rotateSpeed_ = 5.0f;
};