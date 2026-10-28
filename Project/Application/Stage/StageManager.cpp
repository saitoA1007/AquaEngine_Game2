#include "pch.h"
#include "StageManager.h"
#include <numbers>
#include "Floor.h"
#include "TextureManager.h"
using namespace GameEngine;

StageManager::StageManager(GameEngine::GameObjectManager* objectManager, GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager) {
	objectManager_ = objectManager;

	auto* floorModel = modelManager->GetNameByModel("planeXZ.obj");
	auto* wallFractureModel = modelManager->GetNameByModel("wallFracture.gltf");
	auto* wallModel = modelManager->GetNameByModel("wall.obj");
	wallModel->SetDefaultIsEnableLight(true);
	wallModel->SetDefaultColor({ 1,1,1,0.9f });
	wallModel->SetDefaultIOR(1.31f);

	// 地面用の画像を取得
	uint32_t iceNormalGH = textureManager->GetHandleByName("stone_tiles_02_nor_gl_1k.png");
	uint32_t iceHeightGH = textureManager->GetHandleByName("stone_tiles_02_disp_1k.png");
	uint32_t terrainGH = textureManager->GetHandleByName("iceGrass.png");
	uint32_t terrainNormalGH = textureManager->GetHandleByName("aerial_grass_rock_nor_gl_1k.png");

	// 床モデルを生成
	objectManager_->AddObject<Floor>(floorModel, iceNormalGH, iceHeightGH, terrainGH, terrainNormalGH);

	// モデルを取得
	wallModel_ = wallModel;
	wallFractureModel_ = wallFractureModel;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("StageManager");
	RegisterParameter();
	debugParame_->Apply();
	
	// 壁の生成
	GenerateWalls();
}

void StageManager::Initialize() {

}

void StageManager::Update() {
	debugParame_->ApplyIfDirty();

}

void StageManager::Draw() {

}

void StageManager::GenerateWalls() {
	walls_.clear();
	walls_.reserve(maxSideNumber_);

	// 等分した角度を求める
	float centralAngle = std::numbers::pi_v<float> *2.0f / static_cast<float>(maxSideNumber_);

	// 円を生成する
	for (uint32_t i = 0; i < maxSideNumber_; ++i) {
		float angle = i * centralAngle;

		// 円状の位置を求める
		Vector3 anglePos = {
			std::cosf(angle) * radius_,
			0.0f,
			std::sinf(angle) * radius_
		};

		// 生成位置に移動
		Vector3 tmpPos = centerPos_ + anglePos;

		// 壁の向き
		Vector3 dir = centerPos_ - tmpPos;
		float rotateY = std::atan2f(dir.x, dir.z);

		// 座標を設定する
		Transform transform;
		transform.scale = Vector3(1.0f,1.0f,1.0f);
		transform.rotate = Vector3(0.0f, rotateY, 0.0f);
		transform.translate = tmpPos;

		auto* wall = objectManager_->AddObject<Wall>(wallModel_, wallFractureModel_, debugParame_.get());
		wall->SetParameter(transform);

		walls_.push_back(wall);
	}
}

void StageManager::RegisterParameter() {
	int index = 0;
	std::string subGroup = "Generater";
	debugParame_->Register("CenterPos", centerPos_,index++, subGroup);
	debugParame_->Register("MaxSideNumber", maxSideNumber_, index++, subGroup);
	debugParame_->Register("Radius", radius_, index++, subGroup);
}