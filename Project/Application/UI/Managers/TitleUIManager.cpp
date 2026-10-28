#include "pch.h"
#include "TitleUIManager.h"
#include "TextureManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

TitleUIManager::TitleUIManager(GameEngine::TextureManager* textureManager) {

	// テクスチャを取得
	uint32_t titleTextGH = textureManager->GetHandleByName("titleText.png");
	uint32_t buttonTextGH = textureManager->GetHandleByName("spaceText.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("TitleUI");
	debugParame_->RegisterSprite("TitleText", titleTextSprite_);
	debugParame_->RegisterSprite("ButtonText", buttonTextSprite_);
	debugParame_->Apply();

	// テクスチャを設定
	titleTextSprite_.textureHandle_ = titleTextGH;
	buttonTextSprite_.textureHandle_ = buttonTextGH;
}

void TitleUIManager::Initialize() {
	titleTextSprite_.color_.w = 1.0f;
	buttonTextSprite_.color_.w = 1.0f;
	isDraw_ = true;
	isActiveFadeOut_ = false;
}

void TitleUIManager::Update() {
	debugParame_->ApplyIfDirty();

	if (!isDraw_ && isActiveFadeOut_) {
		timer_ += FpsCounter::gameDeltaTime / kFadeOutMaxTime_;

		// 透明にする
		float alpha = Lerp(1.0f, 0.0f, timer_);
		titleTextSprite_.color_.w = alpha;
		buttonTextSprite_.color_.w = alpha;

		if (timer_ >= 1.0f) {
			titleTextSprite_.color_.w = 0.0f;
			buttonTextSprite_.color_.w = 0.0f;
			isActiveFadeOut_ = false;
		}
	}

	// 更新処理
	titleTextSprite_.Update();
	buttonTextSprite_.Update();
}

void TitleUIManager::Draw() {
	if (isDraw_ || isActiveFadeOut_) {
		renderQueue_->SubmitSprite(&titleTextSprite_);
		renderQueue_->SubmitSprite(&buttonTextSprite_);
	}	
}