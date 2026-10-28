#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"
#include "Application/UI/Widgets/TimeUI.h"

// 前方宣言
namespace GameEngine {
	class TextureManager;
}

/// <summary>
/// クリアUIの管理
/// </summary>
class ClearUIManager : public GameEngine::IGameObject {
public:
	ClearUIManager(GameEngine::TextureManager* textureManager);
	~ClearUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 入りのアニメーションを開始
	void StartEnterAnimation() {
		isEnterAnimation_ = true;
		timer_ = 0.0f;
	}

	// 表示する時間を設定
	void SetTime(float time) {
		timeUI_->SetTime(time);
	}

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 時間表示
	std::unique_ptr<TimeUI> timeUI_;

	// 背景
	GameEngine::Sprite bgSprite_;

	// クリア文字UI
	GameEngine::Sprite clearTextSprite_;

	// フレーム
	GameEngine::Sprite frameSprite_;

	// クリア時間文字
	GameEngine::Sprite clearTimeTextSprite_;

	// ボタンUI
	GameEngine::Sprite buttonTextSprite_;

	bool isEnterAnimation_ = false;
	float timer_ = 0.0f;
	float kEnterMaxTime_ = 3.0f;

private:

	// 入りのアニメーション
	void EnterAnimation();
};