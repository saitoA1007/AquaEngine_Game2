#include "pch.h"
#include "ClearUIManager.h"
#include "TextureManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

ClearUIManager::ClearUIManager(GameEngine::TextureManager* textureManager) {

	// クリア
	uint32_t clearTextGH = textureManager->GetHandleByName("clearText.png");
	uint32_t clearTimeTextGH = textureManager->GetHandleByName("clearTimeText.png");
	uint32_t buttonTextGH = textureManager->GetHandleByName("spaceText.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("ClearUI");
	debugParame_->RegisterSprite("BgSprite", bgSprite_);
	debugParame_->RegisterSprite("FrameText", frameSprite_);
	debugParame_->RegisterSprite("ClearText", clearTextSprite_);
	debugParame_->RegisterSprite("ClearTimeText", clearTimeTextSprite_);
	debugParame_->RegisterSprite("ButtonText", buttonTextSprite_);
	debugParame_->Apply();

	// テクスチャを設定
	clearTextSprite_.textureHandle_ = clearTextGH;
	clearTimeTextSprite_.textureHandle_ = clearTimeTextGH;
	buttonTextSprite_.textureHandle_ = buttonTextGH;

	// 時間
	timeUI_ = std::make_unique<TimeUI>("ClearTimeUI", textureManager);

	// 時間を設定
	timeUI_->SetTime(60.0f);
}

void ClearUIManager::Initialize() {
	timeUI_->Initialize();

	// 透明度
	float alpha = 0.0f;
	bgSprite_.color_.w = alpha;
	clearTimeTextSprite_.color_.w = alpha;
	timeUI_->SetAlpha(alpha);
	buttonTextSprite_.color_.w = alpha;
	// 位置を初期化
	frameSprite_.position_.x = 1280.0f;
	clearTextSprite_.position_.x = 1500.0f;

	isEnterAnimation_ = false;
	timer_ = 0.0f;

	Update();
}

void ClearUIManager::Update() {
	debugParame_->ApplyIfDirty();

	if (isEnterAnimation_) {
		EnterAnimation();
	}

	// 更新処理
	bgSprite_.Update();
	frameSprite_.Update();
	clearTextSprite_.Update();
	clearTimeTextSprite_.Update();
	buttonTextSprite_.Update();
	timeUI_->Update();
}

void ClearUIManager::Draw() {

	// 背景
	renderQueue_->SubmitSprite(&bgSprite_);
	// フレーム
	renderQueue_->SubmitSprite(&frameSprite_);
	// クリア文字
	renderQueue_->SubmitSprite(&clearTextSprite_);
	// クリア時間文字
	renderQueue_->SubmitSprite(&clearTimeTextSprite_);
	// ボタン
	renderQueue_->SubmitSprite(&buttonTextSprite_);
	// クリア時間
	timeUI_->Draw();
}

void ClearUIManager::EnterAnimation() {

	timer_ += FpsCounter::gameDeltaTime / kEnterMaxTime_;

	if (timer_ <= 0.2f) {
		// 背景で覆う
		float localT = timer_ / 0.2f;
		// 背景
		bgSprite_.color_.w = Lerp(0.0f, 1.0f, localT);
	} else if(timer_ <= 0.6f) {
		float localT = (timer_ - 0.2f) / 0.4f;
		// フレームの移動
		frameSprite_.position_.x = Lerp(1280.0f,0.0f,localT, EaseType::kEaseOutQuad);
		// クリア文字の移動
		clearTextSprite_.position_.x = Lerp(1500.0f, 640.0f, localT, EaseType::kEaseInQuad);

	} else {
		float localT = (timer_ - 0.6f) / 0.4f;
		float alpha = Lerp(0.0f, 1.0f, localT);
		clearTimeTextSprite_.color_.w = alpha;
		timeUI_->SetAlpha(alpha);
		buttonTextSprite_.color_.w = alpha;
	}

	if (timer_ >= 1.0f) {
		bgSprite_.color_.w = 1.0f;
		frameSprite_.position_.x = 0.0f;
		clearTextSprite_.position_.x = 640.0f;
		isEnterAnimation_ = false;
	}
}