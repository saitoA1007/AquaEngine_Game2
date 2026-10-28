#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"

class ArrowUI : public GameEngine::IGameObject {
public:
	ArrowUI(std::string name, uint32_t texture, GameEngine::Model* model);
	~ArrowUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:

	float maxTime_ = 1.0f;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// モデル
	GameEngine::ModelComponent modelComponent_;

	float timer_ = 0.0f;
};