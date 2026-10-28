#include "pch.h"
#include "ShockFloor.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

ShockFloor::ShockFloor(GameEngine::Model* model, uint32_t crackGH, uint32_t dissolveTexture, Vector3 pos) : modelComponent_(model), floorModelComponent_(model){

	modelComponent_.worldTransform_.transform_.translate = pos;
	modelComponent_.worldTransform_.transform_.scale = { 4.0f,4.0f,4.0f };
	modelComponent_.worldTransform_.transform_.translate.y += 0.01f;
	modelComponent_.worldTransform_.transform_.scale = { 4.0f,4.0f,4.0f };
	floorModelComponent_.worldTransform_.transform_.translate = pos;

	// ディゾルブテクスチャを設定
	modelComponent_.materialData_->dissolveTextureHandle = dissolveTexture;
	modelComponent_.materialData_->textureHandle = crackGH;
	modelComponent_.materialData_->color = { 0.0f,1.0f,1.0f,1.0f };

	// 画像
	floorModelComponent_.materialData_->textureHandle = crackGH;

	modelComponent_.Update();
	floorModelComponent_.Update();
}

void ShockFloor::Initialize() {

}

void ShockFloor::Update() {

	timer_ += FpsCounter::gameDeltaTime / maxTime_;

	modelComponent_.materialData_->dissolveThreshold = Lerp(0.0f, 1.0f, timer_, EaseType::kEaseInQuad);

	if (timer_ >= 1.0f) {

		isDead_ = true;
	}
}

void ShockFloor::Draw() {
	// 描画
	//floorModelComponent_.Draw(renderQueue_, Draw3dType::DefaultAdd);
	modelComponent_.Draw(renderQueue_, Draw3dType::DefaultAdd);
}