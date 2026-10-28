#include "pch.h"
#include "ShockWave.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "MyMath.h"
using namespace GameEngine;

ShockWave::ShockWave(GameEngine::Model* model, GameEngine::Model* planeModel,uint32_t blastGH, uint32_t shockGH, uint32_t dissolveTexture, Vector3 pos) 
	: modelComponent_(model), planeModelComponent_(planeModel) {
	
	// ディゾルブテクスチャを設定
	modelComponent_.materialData_->dissolveTextureHandle = dissolveTexture;

	modelComponent_.materialData_->textureHandle = shockGH;

	modelComponent_.materialData_->color = { 1.0f,1.0f,0.0f,0.8f };

	// 閾値
	modelComponent_.materialData_->dissolveThreshold = 1.0f;

	// 位置を設定
	modelComponent_.worldTransform_.transform_.translate = pos;
	modelComponent_.worldTransform_.transform_.scale = { 6.0f,4.0f,6.0f };
	modelComponent_.Update();

	planeModelComponent_.materialData_->textureHandle = blastGH;
	planeModelComponent_.materialData_->color = { 1.0f,1.0f,0.0f,0.8f };

	Vector3 dir = renderQueue_->GetMainCamera().GetWorldPosition();
	dir.y = 0.0f;
	dir.Normalize();

	planeModelComponent_.worldTransform_.transform_.translate = pos + (dir * 2.0f);
	planeModelComponent_.worldTransform_.transform_.scale = { 4.0f,4.0f,1.0f };
	planeModelComponent_.worldTransform_.UpdateWorldMatrix(Math::MakeBillboardMatrix(planeModelComponent_.worldTransform_.transform_.scale, planeModelComponent_.worldTransform_.transform_.translate, renderQueue_->GetMainCamera().GetWorldMatrix()));	
}

void ShockWave::Initialize() {

}

void ShockWave::Update() {
	modelComponent_.Update();

	timer_ += FpsCounter::gameDeltaTime / maxTime_;

	modelComponent_.materialData_->dissolveThreshold = Lerp(0.0f, 1.0f, timer_, EaseType::kEaseInQuad);

	if (timer_ >= 1.0f) {
		isDead_ = true;
	}
}

void ShockWave::Draw() {
	// 描画
	modelComponent_.Draw(renderQueue_,Draw3dType::DefaultAdd);
	if (timer_ <= 0.4f) {
		planeModelComponent_.Draw(renderQueue_, Draw3dType::DefaultAdd);
	}
	
}