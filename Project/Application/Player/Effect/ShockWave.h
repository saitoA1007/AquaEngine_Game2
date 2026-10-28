#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"

class ShockWave : public GameEngine::IGameObject {
public:
	ShockWave(GameEngine::Model* model, GameEngine::Model* planeModel, uint32_t blastGH, uint32_t shockGH, uint32_t dissolveTexture, Vector3 pos);
	~ShockWave() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	// モデル
	GameEngine::ModelComponent modelComponent_;

	GameEngine::ModelComponent planeModelComponent_;

	float maxTime_ = 0.5f;

	float timer_ = 0.0f;
};