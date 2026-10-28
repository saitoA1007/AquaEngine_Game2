#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "ModelComponent.h"
#include "IceMaterial.h"

class BgIceRock : public GameEngine::IGameObject {
public:
	BgIceRock(GameEngine::Model* model);
	~BgIceRock() = default;

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
};