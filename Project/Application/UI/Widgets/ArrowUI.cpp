#include "pch.h"
#include "ArrowUI.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "MyMath.h"
using namespace GameEngine;

ArrowUI::ArrowUI(std::string name, uint32_t texture, GameEngine::Model* model) : modelComponent_(model) {

	// テクスチャを設定
	modelComponent_.materialData_->textureHandle = texture;
	modelComponent_.materialData_->color = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name);
	debugParame_->RegisterWorld("Arrow", modelComponent_.worldTransform_);
	debugParame_->Register("MaxTime", maxTime_);
	debugParame_->Apply();
}

void ArrowUI::Initialize() {
	
}

void ArrowUI::Update() {
	debugParame_->ApplyIfDirty();

	timer_ += FpsCounter::gameDeltaTime / maxTime_;

	float offsetY = 0.0f;
	if (timer_ <= 0.5f) {
		float localT = timer_ / 0.5f;

		offsetY = Lerp(-1.0f, 1.0f,localT, EaseType::kEaseInQuad);
	} else {
		float localT = (timer_ - 0.5f) / 0.5f;
		offsetY = Lerp(1.0f, -1.0f, localT, EaseType::kEaseInQuad);
	}

	if (timer_ >= 1.0f) {
		timer_ = 0.0f;
	}

	// カメラのワールド行列を取得
	Matrix4x4 cameraMatrix = renderQueue_->GetMainCamera().GetWorldMatrix();
	Matrix4x4 billboardMatrix = Math::MakeBillboardMatrix(
		modelComponent_.worldTransform_.transform_.scale,
		modelComponent_.worldTransform_.transform_.translate + Vector3(0.0f, offsetY, 0.0f),
		modelComponent_.worldTransform_.transform_.rotate.z, cameraMatrix);

	// 更新
	modelComponent_.worldTransform_.SetWorldMatrix(billboardMatrix);
}

void ArrowUI::Draw() {
	modelComponent_.Draw(renderQueue_);
}