#pragma once
#include <vector>
#include "IGameObject.h"
#include "Model.h"
#include "GameObjectManager.h"
#include "ModelManager.h"
#include "Wall.h"
#include "DebugParameter.h"

// 前方宣言
namespace GameEngine {
	class TextureManager;
}

class StageManager : public GameEngine::IGameObject {
public:
	StageManager(GameEngine::GameObjectManager* objectManager, GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager);
	~StageManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	GameEngine::GameObjectManager* objectManager_ = nullptr;
	GameEngine::Model* wallModel_ = nullptr;
	GameEngine::Model* wallFractureModel_ = nullptr;

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 生成位置
	Vector3 centerPos_ = { 0.0f,1.0f,0.0f };
	// 辺の数
	uint32_t maxSideNumber_ = 8;
	// 半径
	float radius_ = 20.0f;
	
	// 壁のデータ
	std::vector<Wall*> walls_;

private:

	// ステージを生成する
	void GenerateWalls();

	// 値を登録
	void RegisterParameter();

};