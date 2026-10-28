#include "pch.h"
#include "TimeUI.h"
#include <algorithm>
#include "TextureManager.h"
using namespace GameEngine;

TimeUI::TimeUI(std::string name, GameEngine::TextureManager* textureManager) {

	// テクスチャを取得
	for (size_t i = 0; i < numTexture_.size(); ++i) {
		numGH_[i] = textureManager->GetHandleByName(numTexture_[i]);
	}

	// 画像を生成
	for (size_t i = 0; i < numSprite_.size(); ++i) {
		numSprite_[i].sprite_ = std::make_unique<Sprite>();
		numSprite_[i].sprite_->SetParent(&baseWorld_);
		numSprite_[i].num_ = numGH_[0];
	}

	// 中間点
	dotSprite_ = std::make_unique<Sprite>();
	dotSprite_->SetParent(&baseWorld_);
	dotSprite_->textureHandle_ = textureManager->GetHandleByName("dotto.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name);
	debugParame_->RegisterWorld("Base", baseWorld_);
	for (size_t i = 0; i < numSprite_.size(); ++i) {
		std::string numName = "NumSprite" + std::to_string(i);
		debugParame_->RegisterSprite(numName, *numSprite_[i].sprite_);
	}
	debugParame_->RegisterSprite("DotSprite", *dotSprite_);
	debugParame_->Apply();
}

void TimeUI::Initialize() {

}

void TimeUI::Update() {
	debugParame_->ApplyIfDirty();

	// 更新
	baseWorld_.UpdateTransformMatrix();

	dotSprite_->Update();

	for (auto& sprite : numSprite_) {
		sprite.sprite_->Update();
	}
}

void TimeUI::Draw() {

	// 数を描画
	for (auto& sprite : numSprite_) {
		renderQueue_->SubmitSprite(sprite.sprite_.get());
	}

	// 中間点
	renderQueue_->SubmitSprite(dotSprite_.get());
}

void TimeUI::CalculateTotalCount() {
	// 制限する
	time_ = std::clamp(time_, 0.0f, 5999.0f);

	int totalSeconds = static_cast<int>(time_);

	int minutes = totalSeconds / 60;
	int seconds = totalSeconds % 60;

	for (size_t i = 0; i < numSprite_.size(); ++i) {
		if (i == 0) {
			// 分の10の位
			numSprite_[i].num_ = minutes / 10;
		} else if (i == 1) {
			// 分の1の位
			numSprite_[i].num_ = minutes % 10;
		} else if (i == 2) {
			// 秒の10の位
			numSprite_[i].num_ = seconds / 10;
		} else if (i == 3) {
			// 秒の1の位
			numSprite_[i].num_ = seconds % 10;
		}
	}

	// 画像を設定する
	for (size_t i = 0; i < numSprite_.size(); ++i) {
		numSprite_[i].sprite_->textureHandle_ = numGH_[numSprite_[i].num_];
	}
}