#include "pch.h"
#include "LetterboxUI.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

LetterboxUI::LetterboxUI(std::string name) {

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name);
	debugParame_->RegisterWorld("Base", baseWorld_);
	debugParame_->RegisterSprite("UpBar", upBarSprite_);
	debugParame_->RegisterSprite("DownBar", downBarSprite_);
	debugParame_->Register("UpBarStartPosY", upBarStartPosY_, 0);
	debugParame_->Register("UpBarEndPosY", upBarEndPosY_, 1);
	debugParame_->Register("DownBarStartPosY", downBarStartPosY_, 2);
	debugParame_->Register("DownBarEndPosY", downBarEndPosY_, 3);
	debugParame_->Register("MaxTime", maxTime_, 4);
	debugParame_->Apply();

	// ペアレント
	upBarSprite_.SetParent(&baseWorld_);
	downBarSprite_.SetParent(&baseWorld_);
}

void LetterboxUI::Initialize() {
	debugParame_->Apply();

	timer_ = 1.0f;
	isBarActive_ = false;
	upBarSprite_.position_.y = upBarEndPosY_;
	downBarSprite_.position_.y = downBarEndPosY_;
}

void LetterboxUI::Update() {
	debugParame_->ApplyIfDirty();

	// バーを動かす
	BarAnimation();
	
	// 更新処理
	baseWorld_.UpdateTransformMatrix();
	upBarSprite_.Update();
	downBarSprite_.Update();
}

void LetterboxUI::Draw() {

	// 描画
	renderQueue_->SubmitSprite(&upBarSprite_);
	renderQueue_->SubmitSprite(&downBarSprite_);
}

void LetterboxUI::BarAnimation() {
	if (isBarActive_) {
		// 帯を出現させる
		if (timer_ >= 1.0f) { return; }
		timer_ += FpsCounter::gameDeltaTime / maxTime_;

		upBarSprite_.position_.y = Lerp(upBarEndPosY_, upBarStartPosY_, timer_, EaseType::kEaseInQuad);
		downBarSprite_.position_.y = Lerp(downBarEndPosY_, downBarStartPosY_, timer_, EaseType::kEaseInQuad);

		if (timer_ >= 1.0f) {
			upBarSprite_.position_.y = upBarStartPosY_;
			downBarSprite_.position_.y = downBarStartPosY_;
		}
	} else {
		// 帯を元に戻す
		if (timer_ >= 1.0f) { return; }
		timer_ += FpsCounter::gameDeltaTime / maxTime_;

		upBarSprite_.position_.y = Lerp(upBarStartPosY_, upBarEndPosY_, timer_, EaseType::kEaseOutQuad);
		downBarSprite_.position_.y = Lerp(downBarStartPosY_, downBarEndPosY_, timer_, EaseType::kEaseOutQuad);
		if (timer_ >= 1.0f) {
			upBarSprite_.position_.y = upBarEndPosY_;
			downBarSprite_.position_.y = downBarEndPosY_;
		}
	}

}