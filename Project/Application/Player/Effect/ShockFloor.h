#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"

class ShockFloor : public GameEngine::IGameObject {
public:
	ShockFloor(GameEngine::Model* model, uint32_t crackGH, uint32_t dissolveTexture, Vector3 pos);
	~ShockFloor() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	// モデル
	GameEngine::ModelComponent modelComponent_;

	// 地面モデル
	GameEngine::ModelComponent floorModelComponent_;

	float maxTime_ = 1.8f;

	float timer_ = 0.0f;
};