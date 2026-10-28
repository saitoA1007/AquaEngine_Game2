#pragma once
#include "IGameObject.h"
#include "Application/UI/Widgets/HpBarUI.h"
#include "Application/UI/Widgets/HpContainer.h"
#include "Application/UI/Widgets/ArrowUI.h"
#include "Application/UI/Effects/LetterboxUI.h"

// 前方宣言
namespace GameEngine {
	class TextureManager;
}

/// <summary>
/// プレイUIの管理
/// </summary>
class PlayUIManager : public GameEngine::IGameObject {
public:
	PlayUIManager(GameEngine::TextureManager* textureManager, GameEngine::Model* planeModel);
	~PlayUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	void SetCurrentBossHp(int32_t hp) {
		bossHpBarUI_->SetCurrentHp(hp);
	}

	void SetMaxBossHp(int32_t hp) {
		bossHpBarUI_->SetMaxHp(hp);
	}

	void SetCurrentPlayerHp(int32_t hp) {
		playerHpUI_->SetCurrentHp(hp);
	}

	void SetMaxPlayerHp(int32_t hp) {
		playerHpUI_->SetMaxHp(hp);
	}

	void SetBarActive(bool isActive) {
		letterBoxUI_->SetBarActive(isActive);
	}

	// プレイヤーや敵のHpUIなどの表示フラグ
	void SetIsDrawGamePlayUI(bool isDraw) {
		isDrawGamePlayUI_ = isDraw;
	}

	// 操作方法UIの表示フラグ
	void SetIsDrawPlayGuide(bool isDraw) {
		isDrawPlayGuide_ = isDraw;
	}

	// チュートリアルの表示フラグ
	void SetIsDrawTutorialGuide(bool isDraw) {
		isDrawTutorialGuide_ = isDraw;
	}

	// 矢印の表示フラグ
	void SetIsDrawArrowUI(bool isDraw) {
		isDrawArrowUI_ = isDraw;
	}

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 表示フラグ
	bool isDrawPlayGuide_ = true;
	bool isDrawTutorialGuide_ = true;
	bool isDrawGamePlayUI_ = true;
	bool isDrawArrowUI_ = false;

	// ボスのHpUI
	std::unique_ptr<HpBarUI> bossHpBarUI_;

	// ボスの名前
	GameEngine::Sprite bossNameSprite_;

	// プレイヤーHpUI
	std::unique_ptr<HpContainer> playerHpUI_;

	// 操作説明UI
	GameEngine::Sprite playGuideSprite_;

	// チュートリアルUI
	std::vector<GameEngine::Sprite> tutorialTextSprites_;

	// 黒帯表示
	std::unique_ptr<LetterboxUI> letterBoxUI_;

	// 矢印ui
	std::unique_ptr<ArrowUI> arrowUI_;
};