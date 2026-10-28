#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"
#include "Application/UI/Effects/DimmerUI.h"

// 前方宣言
namespace GameEngine {
	class TextureManager;
}

/// <summary>
/// ゲームオーバーUIの管理
/// </summary>
class GameOverUIManager : public GameEngine::IGameObject {
public:

	enum class SelectType {
		kRetry,     // やり直す
		kBackTitle, // タイトルに戻る

		kMaxCount
	};

public:
	GameOverUIManager(GameEngine::TextureManager* textureManager);
	~GameOverUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// アニメーションさせる
	void Play() {
		if (type_ == SelectType::kRetry) {
			retryUI_->Play();
		} else {
			backTitleUI_->Play();
		}
	}

	// タイプを取得
	SelectType GetType() const { return type_; }

	// 選択するタイプを設定する
	void SetType(SelectType type) {
		type_ = type;
	}

	// 入りのアニメーションを開始
	void StartEnterAnimation() {
		isEnterAnimation_ = true;
		timer_ = 0.0f;
	}

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ゲームオーバー文字UI
	GameEngine::Sprite gameOverTextSprite_;

	// 背景
	GameEngine::Sprite bgSprite_;

	// やり直す
	std::unique_ptr<DimmerUI> retryUI_;

	// タイトルへ戻る
	std::unique_ptr<DimmerUI> backTitleUI_;

	// 選択中のタイプ
	SelectType type_ = SelectType::kRetry;

	bool isEnterAnimation_ = false;
	float timer_ = 0.0f;
	float kEnterMaxTime_ = 1.0f;

private:

	// 入りのアニメーション
	void EnterAnimation();
};