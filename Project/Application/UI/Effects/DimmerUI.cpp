#include "pch.h"
#include "DimmerUI.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

DimmerUI::DimmerUI(std::string name, GameEngine::DebugParameter* debugParame, bool isFrameActive) {
	name_ = name;

	isFrameActive_ = isFrameActive;

	int i = 0;
	debugParame->Register("MaxTime", maxTime_, i++, name_);
	debugParame->Register("ScaleRatio", scaleRatio_, i++, name_);
	debugParame->RegisterSprite("Text", sprite_, name_);
	debugParame->RegisterWorld("BaseWorld", world_, name_);
	sprite_.SetParent(&world_);

	// フレームを有効化
	if (isFrameActive_) {
		frameSprite_ = std::make_unique<Sprite>();
		debugParame->RegisterSprite("Frame", *frameSprite_, name_);
		frameSprite_->SetParent(&world_);
	}
}

void DimmerUI::Initialize() {
	startScale_ = world_.transform_.scale;
	endScale_ = startScale_ * scaleRatio_;

	timer_ = 0.0f;
	isPlay_ = false;
}

void DimmerUI::Update() {

	if (isPlay_) {
		timer_ += FpsCounter::gameDeltaTime / maxTime_;

		if (timer_ <= 0.5f) {
			float localT = timer_ / 0.5f;
			world_.transform_.scale = Lerp(startScale_, endScale_, localT, EaseType::kEaseInOutQuad);
		} else {
			float localT = (timer_ - 0.5f) / 0.5f;
			world_.transform_.scale = Lerp(endScale_, startScale_, localT, EaseType::kEaseInOutQuad);
		}

		if (timer_ >= 1.0f) {
			world_.transform_.scale = startScale_;
			timer_ = 0.0f;
			isPlay_ = false;
		}
	}

	// 更新
	world_.UpdateTransformMatrix();
	sprite_.Update();
	if (isFrameActive_) {
		frameSprite_->Update();
	}
}

void DimmerUI::Draw() {

	// フレーム描画
	if (isFrameActive_) {
		renderQueue_->SubmitSprite(frameSprite_.get());
	}

	// 描画
	renderQueue_->SubmitSprite(&sprite_);
}