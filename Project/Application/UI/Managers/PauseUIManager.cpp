#include "pch.h"
#include "PauseUIManager.h"
#include "TextureManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

PauseUIManager::PauseUIManager(GameEngine::TextureManager* textureManager) {

	// ポーズ
	uint32_t pauseTextGH = textureManager->GetHandleByName("pauseText.png");
	uint32_t backTextGH = textureManager->GetHandleByName("backText.png");
	uint32_t retryTextGH = textureManager->GetHandleByName("RetryText.png");
	uint32_t backTitleTextGH = textureManager->GetHandleByName("BackTitleText.png");
	uint32_t playGuideGH = textureManager->GetHandleByName("playGuide.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("PauseUI");
	debugParame_->RegisterSprite("Bg", bgSprite_);
	debugParame_->RegisterSprite("Frame", frameSprite_);
	debugParame_->RegisterSprite("PlayGuide", playGuideSprite_);
	debugParame_->RegisterSprite("PauseText", pauseTextSprite_);

	// 戻る
	backUI_ = std::make_unique<DimmerUI>("BackUI", debugParame_.get(), true);
	// リトライ
	retryUI_ = std::make_unique<DimmerUI>("RetryUI", debugParame_.get(),true);
	// タイトルへ
	backTitleUI_ = std::make_unique<DimmerUI>("BackTitleUI", debugParame_.get(),true);

	// パラメーターを適応
	debugParame_->Apply();

	// テクスチャを設定
	pauseTextSprite_.textureHandle_ = pauseTextGH;
	playGuideSprite_.textureHandle_ = playGuideGH;
	backUI_->SetTexture(backTextGH);
	retryUI_->SetTexture(retryTextGH);
	backTitleUI_->SetTexture(backTitleTextGH);
}

void PauseUIManager::Initialize() {
	bgSprite_.color_.w = 0.0f;
	playGuideSprite_.color_.w = 0.0f;
	// 初期化位置
	frameSprite_.position_.x = -384.0f;
	pauseTextSprite_.position_.x = -384.0f;
	backUI_->world_.transform_.translate.x = -256.0f;
	retryUI_->world_.transform_.translate.x = -256.0f;
	backTitleUI_->world_.transform_.translate.x = -256.0f;
}

void PauseUIManager::Update() {
	debugParame_->ApplyIfDirty();

	// アニメーション
	if(isAnimation_) {

		Animation();
	} else {
		switch (type_)
		{
		case PauseUIManager::SelectType::kBack:
			backUI_->world_.transform_.translate.x = 128.0f;
			retryUI_->world_.transform_.translate.x = 96.0f;
			backTitleUI_->world_.transform_.translate.x = 96.0f;
			break;

		case PauseUIManager::SelectType::kRetry:
			backUI_->world_.transform_.translate.x = 96.0f;
			retryUI_->world_.transform_.translate.x = 128.0f;
			backTitleUI_->world_.transform_.translate.x = 96.0f;
			break;

		case PauseUIManager::SelectType::kBackTitle:
			backUI_->world_.transform_.translate.x = 96.0f;
			retryUI_->world_.transform_.translate.x = 96.0f;
			backTitleUI_->world_.transform_.translate.x = 128.0f;
			break;
		}
	}

	bgSprite_.Update();
	frameSprite_.Update();
	playGuideSprite_.Update();
	backUI_->Update();
	retryUI_->Update();
	backTitleUI_->Update();
	pauseTextSprite_.Update();
}

void PauseUIManager::Draw() {

	renderQueue_->SubmitSprite(&bgSprite_);
	renderQueue_->SubmitSprite(&frameSprite_);
	renderQueue_->SubmitSprite(&playGuideSprite_);
	renderQueue_->SubmitSprite(&pauseTextSprite_);
	backUI_->Draw();
	retryUI_->Draw();
	backTitleUI_->Draw();
}

void PauseUIManager::Animation() {

	switch (phase_)
	{
	case PauseUIManager::Phase::kIn:
		timer_ += FpsCounter::deltaTime / kInMaxTime_;

		// 背景の表示
		if (timer_ <= 0.5f) {
			float localT = timer_ / 0.5f;
			bgSprite_.color_.w = Lerp(0.0f, 0.5f, localT);
		}

		// フレームの移動
		if (timer_ <= 0.8f) {
			float localT = timer_ / 0.8f;
			frameSprite_.position_.x = Lerp(-384.0f, 0.0f, localT, EaseType::kEaseOutQuad);
		}
		// ポーズ文字の移動
		if (timer_ <= 0.9f) {
			float localT = timer_ / 0.9f;
			pauseTextSprite_.position_.x = Lerp(-384.0f, 192.0f, localT, EaseType::kEaseOutQuad);
		}
		
		// 選択テキストの移動
		backUI_->world_.transform_.translate.x = Lerp(-256.0f, 128.0f, timer_, EaseType::kEaseInQuad);
		retryUI_->world_.transform_.translate.x = Lerp(-256.0f, 96.0f, timer_, EaseType::kEaseInQuad);
		backTitleUI_->world_.transform_.translate.x = Lerp(-256.0f, 96.0f, timer_, EaseType::kEaseInQuad);

		// 操作説明の表示
		playGuideSprite_.color_.w = Lerp(0.0f, 1.0f, timer_, EaseType::kEaseInQuad);

		if (timer_ >= 1.0f) {
			frameSprite_.position_.x = 0.0f;
			pauseTextSprite_.position_.x = 192.0f;
			backUI_->world_.transform_.translate.x = 128.0f;
			retryUI_->world_.transform_.translate.x = 96.0f;
			backTitleUI_->world_.transform_.translate.x = 96.0f;
			playGuideSprite_.color_.w = 1.0f;
			isAnimation_ = false;
		}
		break;

	case PauseUIManager::Phase::kOut:
		timer_ += FpsCounter::deltaTime / kOutMaxTime_;

		// 背景の表示
		bgSprite_.color_.w = Lerp(0.5f, 0.0f, timer_);

		// フレームの移動
		frameSprite_.position_.x = Lerp(0.0f, -384.0f, timer_, EaseType::kEaseInQuad);

		// ポーズ文字の移動
		if (timer_ <= 0.9f) {
			float localT = timer_ / 0.9f;
			pauseTextSprite_.position_.x = Lerp(192.0f, -384.0f, localT, EaseType::kEaseOutQuad);
		}
		// 選択テキストの移動
		if (timer_ <= 0.8f) {
			float localT = timer_ / 0.8f;
			// 選択テキストの移動
			backUI_->world_.transform_.translate.x = Lerp(96.0f, -256.0f, localT, EaseType::kEaseOutQuad);
			retryUI_->world_.transform_.translate.x = Lerp(96.0f, -256.0f, localT, EaseType::kEaseOutQuad);
			backTitleUI_->world_.transform_.translate.x = Lerp(96.0f, -256.0f, localT, EaseType::kEaseOutQuad);

			// 操作説明の表示
			playGuideSprite_.color_.w = Lerp(1.0f, 0.0f, localT, EaseType::kEaseOutQuad);
		}

		if (timer_ >= 1.0f) {
			playGuideSprite_.color_.w = 0.0f;
			backUI_->world_.transform_.translate.x = -256.0f;
			retryUI_->world_.transform_.translate.x = -256.0f;
			backTitleUI_->world_.transform_.translate.x = -256.0f;
			frameSprite_.position_.x = -384.0f;
			isAnimation_ = false;
		}
		break;
	}
}