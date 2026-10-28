#include "pch.h"
#include "PlayUIManager.h"
#include "TextureManager.h"
using namespace GameEngine;

PlayUIManager::PlayUIManager(GameEngine::TextureManager* textureManager, GameEngine::Model* planeModel) {

	// テクスチャを朱徳
	uint32_t playerHpGH = textureManager->GetHandleByName("PlayerHP.png");
	uint32_t bossNameGH = textureManager->GetHandleByName("BossName.png");
	uint32_t pauseGuideGH = textureManager->GetHandleByName("pauseGuide.png");
	uint32_t tutorial0GH = textureManager->GetHandleByName("Tutorial_01.png");
	uint32_t tutorial1GH = textureManager->GetHandleByName("Tutorial_02.png");
	uint32_t tutorial2GH = textureManager->GetHandleByName("Tutorial_03.png");
	uint32_t arrowGH = textureManager->GetHandleByName("arrow.png");

	// チュートリアルの表示文字
	tutorialTextSprites_.resize(3);

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("PlayUI");
	debugParame_->RegisterSprite("BossName", bossNameSprite_);
	debugParame_->RegisterSprite("PlayGuide", playGuideSprite_);
	// チュートリアルテキスト
	for (uint32_t i = 0; i < tutorialTextSprites_.size(); ++i) {
		std::string num = std::to_string(i);
		debugParame_->RegisterSprite("TutorialText" + num, tutorialTextSprites_[i]);

		// 画像を設定
		if (i == 0) {
			tutorialTextSprites_[i].textureHandle_ = tutorial0GH;
		} else if (i == 1) {
			tutorialTextSprites_[i].textureHandle_ = tutorial1GH;
		} else {
			tutorialTextSprites_[i].textureHandle_ = tutorial2GH;
		}
	}
	debugParame_->Apply();

	// テクスチャを設定
	bossNameSprite_.textureHandle_ = bossNameGH;
	playGuideSprite_.textureHandle_ = pauseGuideGH;

	// ボスUI
	bossHpBarUI_ = std::make_unique<HpBarUI>("BossHpUI");

	// プレイヤーUI
	playerHpUI_ = std::make_unique<HpContainer>("PlayerHpUI", playerHpGH);

	// 黒帯UI
	letterBoxUI_ = std::make_unique<LetterboxUI>("LetterboxUI");

	// 矢印
	arrowUI_ = std::make_unique<ArrowUI>("ArrowUI", arrowGH, planeModel);
}

void PlayUIManager::Initialize() {
	bossHpBarUI_->Initialize();
	playerHpUI_->Initialize();
	letterBoxUI_->Initialize();
	arrowUI_->Initialize();

	// 更新して適応させる
	Update();

	isDrawGamePlayUI_ = false;
	isDrawPlayGuide_ = false;
	isDrawTutorialGuide_ = false;
}

void PlayUIManager::Update() {
	debugParame_->ApplyIfDirty();

	bossNameSprite_.Update();
	playGuideSprite_.Update();
	bossHpBarUI_->Update();
	playerHpUI_->Update();
	letterBoxUI_->Update();
	arrowUI_->Update();

	for (auto& sprite : tutorialTextSprites_) {
		sprite.Update();
	}
}

void PlayUIManager::Draw() {
	// 黒帯を描画
	letterBoxUI_->Draw();

	// 矢印
	if (isDrawArrowUI_) {
		arrowUI_->Draw();
	}

	if (isDrawGamePlayUI_) {
		// ボスHpを描画
		bossHpBarUI_->Draw();
		renderQueue_->SubmitSprite(&bossNameSprite_);
		// プレイヤーHpを描画
		playerHpUI_->Draw();
	}
	
	// 操作方法
	if (isDrawPlayGuide_) {
		renderQueue_->SubmitSprite(&playGuideSprite_);
	}

	// チュートリアル
	if (isDrawTutorialGuide_) {
		for (auto& sprite : tutorialTextSprites_) {
			renderQueue_->SubmitSprite(&sprite);
		}
	}
}