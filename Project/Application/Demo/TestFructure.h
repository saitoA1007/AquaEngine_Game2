#pragma once
#include "IGameObject.h"
#include "Collider.h"
#include "DebugParameter.h"
#include "DestructibleObject.h"

class TestFructure : public GameEngine::IGameObject {
public:
	TestFructure(GameEngine::Model* model);
	~TestFructure() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	// パラメータ機能
	GameEngine::DebugParameter debugParame_{ "TestFructure" };

	// 球の当たり判定
	GameEngine::SphereCollider testCollider_;
	Vector3 testPos_ = { 3.0f,0.0f,0.0f };
	float radius_ = 1.0f;

	// 破片
	GameEngine::DestructibleObject destructibleObject_;
};