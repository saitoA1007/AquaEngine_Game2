#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"

// 前方宣言
namespace GameEngine {
	class TextureManager;
}

/// <summary>
/// タイトルUIの管理
/// </summary>
class TitleUIManager : public GameEngine::IGameObject {
public:
	TitleUIManager(GameEngine::TextureManager* textureManager);
	~TitleUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 表示の有効設定
	void SetIsDraw(bool isActive) {
		isDraw_ = isActive;

		if (isDraw_) {
			titleTextSprite_.color_.w = 1.0f;
			buttonTextSprite_.color_.w = 1.0f;
		} else {
			//　フェードアウトして非表示にする
			isActiveFadeOut_ = true;
		}
	}

	bool IsDraw() const { return isDraw_; }
	bool IsActiveFadeOut() const { return isActiveFadeOut_; }

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// タイトル文字UI
	GameEngine::Sprite titleTextSprite_;

	// ボタンUI
	GameEngine::Sprite buttonTextSprite_;

	bool isDraw_ = true;
	bool isActiveFadeOut_ = false;

	float timer_ = 0.0f;
	float kFadeOutMaxTime_ = 1.0f;
};