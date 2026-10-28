#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "IceMaterial.h"

class IceDemo : public GameEngine::IGameObject {
public:
	IceDemo(std::string name, GameEngine::Model* model);
	~IceDemo() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

	// デバックの更新
	void DebugUpdate() override;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 氷のマテリアル
	GameEngine::IceMaterial iceMaterial_;

	// モデル
	GameEngine::ModelComponent modelComponent_;
};