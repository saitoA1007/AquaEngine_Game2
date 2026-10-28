#include "pch.h"
#include "GameOverUIManager.h"
#include "TextureManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

GameOverUIManager::GameOverUIManager(GameEngine::TextureManager* textureManager) {

	// ゲームオーバー
	uint32_t gameOverTextGH = textureManager->GetHandleByName("GameOverText.png");
	uint32_t retryTextGH = textureManager->GetHandleByName("RetryText.png");
	uint32_t backTitleTextGH = textureManager->GetHandleByName("BackTitleText.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("GameOverUI");
	debugParame_->RegisterSprite("GameOverText", gameOverTextSprite_);
	debugParame_->RegisterSprite("Bg", bgSprite_);

	// リトライ
	retryUI_ = std::make_unique<DimmerUI>("RetryUI", debugParame_.get());
	// タイトルへ
	backTitleUI_ = std::make_unique<DimmerUI>("BackTitleUI", debugParame_.get());

	// パラメーターを適応
	debugParame_->Apply();

	// テクスチャを設定
	gameOverTextSprite_.textureHandle_ = gameOverTextGH;
	retryUI_->SetTexture(retryTextGH);
	backTitleUI_->SetTexture(backTitleTextGH);
}

void GameOverUIManager::Initialize() {
	retryUI_->Initialize();
	backTitleUI_->Initialize();

	// 透明度をリセット
	float alpha = 0.0f;
	retryUI_->SetColor(Vector4(0.9f, 0.9f, 0.9f, alpha));
	backTitleUI_->SetColor(Vector4(0.5f, 0.5f, 0.5f, alpha));
	gameOverTextSprite_.color_.w = alpha;
	bgSprite_.color_.w = alpha;

	// リトライに設定
	type_ = SelectType::kRetry;

	// 更新して適応させる
	Update();
}

void GameOverUIManager::Update() {
	debugParame_->ApplyIfDirty();

	if (isEnterAnimation_) {
		EnterAnimation();
	}

	switch (type_)
	{
	case GameOverUIManager::SelectType::kRetry:
		retryUI_->SetColor(Vector3(0.9f,0.9f,0.9f));
		backTitleUI_->SetColor(Vector3(0.5f,0.5f,0.5f));
		break;

	case GameOverUIManager::SelectType::kBackTitle:
		retryUI_->SetColor(Vector3(0.5f, 0.5f, 0.5f));
		backTitleUI_->SetColor(Vector3(0.9f, 0.9f, 0.9f));
		break;
	}

	// 更新
	retryUI_->Update();
	backTitleUI_->Update();
	gameOverTextSprite_.Update();
	bgSprite_.Update();
}

void GameOverUIManager::Draw() {
	// 描画
	renderQueue_->SubmitSprite(&bgSprite_);
	renderQueue_->SubmitSprite(&gameOverTextSprite_);
	retryUI_->Draw();
	backTitleUI_->Draw();
}

void GameOverUIManager::EnterAnimation() {
	timer_ += FpsCounter::gameDeltaTime / kEnterMaxTime_;

	// 透明度
	float alpha = Lerp(0.0f, 1.0f, timer_);
	retryUI_->SetAlpha(alpha);
	backTitleUI_->SetAlpha(alpha);
	gameOverTextSprite_.color_.w = alpha;
	bgSprite_.color_.w = alpha;

	if (timer_ >= 1.0f) {
		alpha = 1.0f;
		retryUI_->SetAlpha(alpha);
		backTitleUI_->SetAlpha(alpha);
		gameOverTextSprite_.color_.w = alpha;
		bgSprite_.color_.w = alpha;
		isEnterAnimation_ = false;
	}
}
	