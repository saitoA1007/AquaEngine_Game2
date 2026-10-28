#include "pch.h"
#include "PlayerHitAttackEffect.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "MyMath.h"
using namespace GameEngine;

PlayerHitAttackEffect::PlayerHitAttackEffect(uint32_t texture, GameEngine::Model* planeModel) : modelComponent_(planeModel) {

	// 設定
	modelComponent_.materialData_->textureHandle = texture;
	modelComponent_.materialData_->enableLighting = false;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("PlayerHitAttackEffect");
	debugParame_->Register("MaxTime", maxTime_);
	debugParame_->Register("RotateSpeed", rotateSpeed_);
	debugParame_->Apply();
}

void PlayerHitAttackEffect::Initialize() {

}

void PlayerHitAttackEffect::Update() {
	debugParame_->ApplyIfDirty();

	timer_ += FpsCounter::gameDeltaTime / maxTime_;

	if (timer_ >= 1.0f) {
		isActive_ = false;
	}

	// 回転
	modelComponent_.worldTransform_.transform_.rotate.z += rotateSpeed_ * FpsCounter::gameDeltaTime;

	// カメラのワールド行列を取得
	Matrix4x4 cameraMatrix = renderQueue_->GetMainCamera().GetWorldMatrix();
	Matrix4x4 billboardMatrix = Math::MakeBillboardMatrix(
		modelComponent_.worldTransform_.transform_.scale,
		modelComponent_.worldTransform_.transform_.translate,
		modelComponent_.worldTransform_.transform_.rotate.z, cameraMatrix);

	// 更新
	modelComponent_.worldTransform_.SetWorldMatrix(billboardMatrix);
}

void PlayerHitAttackEffect::Draw() {
	// 描画
	modelComponent_.Draw(renderQueue_,Draw3dType::DefaultAdd);
}