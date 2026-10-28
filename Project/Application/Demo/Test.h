#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"

class Test : public GameEngine::IGameObject {
public:
	Test(GameEngine::Model* model);
	~Test() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	// パラメータ機能
	GameEngine::DebugParameter debugParame_{"Test"};

	float testFloat_ = 0.0f;
	Vector3 testPos_ = {};

	// モデル
	GameEngine::ModelComponent modelComponent_;
};