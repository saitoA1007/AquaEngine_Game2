#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "IceMaterial.h"
#include "DestructibleObject.h"
#include "Collider.h"
#include "InputCommand.h"

class FructureDemo : public GameEngine::IGameObject {
public:
	FructureDemo(std::string name, GameEngine::InputCommand* inputCommand, GameEngine::Model* model);
	~FructureDemo() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

	// デバックの更新
	void DebugUpdate() override;

public:

	// リセット
	void Reset() {
		destructibleObject_.Reassemble();
	}

private:
	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 氷のマテリアル
	GameEngine::IceMaterial iceMaterial_;

	// 破片
	GameEngine::DestructibleObject destructibleObject_;

	// カメラ基準
	Vector3 cameraForwardXZ_ = { 0.0f,0.0f,1.0f };
	Vector3 cameraRightXZ_ = { 1.0f,0.0f,0.0f };

	// 球の当たり判定
	GameEngine::SphereCollider testCollider_;
	Vector3 testPos_ = { 3.0f,0.0f,0.0f };
	float radius_ = 1.0f;;

private:

	// 移動
	void Move();

	void UpdateCameraBasis();
};