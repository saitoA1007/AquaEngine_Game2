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
/// ポーズUIの管理
/// </summary>
class PauseUIManager : public GameEngine::IGameObject {
public:
	
	// 選択している状態
	enum class SelectType {
		kBack,      // 戻る
		kRetry,     // やり直す
		kBackTitle, // タイトルに戻る

		kMaxCount
	};

	// 現在のフェーズ
	enum class Phase {
		kIn,
		kOut,
	};

public:
	PauseUIManager(GameEngine::TextureManager* textureManager);
	~PauseUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// アニメーションを開始する
	void Play(Phase phase) {
		isAnimation_ = true;
		timer_ = 0.0f;
		phase_ = phase;
	}

	bool IsAnimation() const { return isAnimation_; }

	Phase GetPhase() const { return phase_; }

	SelectType GetType() const { return type_; }

	SelectType SetType(SelectType type) { 
		return type_ = type; 
	}

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ポーズ文字UI
	GameEngine::Sprite pauseTextSprite_;

	// 半透明の背景
	GameEngine::Sprite bgSprite_;

	// 隠れるフレーム部分
	GameEngine::Sprite frameSprite_;

	// 操作説明
	GameEngine::Sprite playGuideSprite_;

	// 戻る
	std::unique_ptr<DimmerUI> backUI_;

	// やり直す
	std::unique_ptr<DimmerUI> retryUI_;

	// タイトルへ戻る
	std::unique_ptr<DimmerUI> backTitleUI_;

	// 選択中のタイプ
	SelectType type_ = SelectType::kBack;

	// 現在の状態
	Phase phase_ = Phase::kIn;
	bool isAnimation_ = false;

	float timer_ = 0.0f;
	float kInMaxTime_ = 1.0f;
	float kOutMaxTime_ = 0.8f;

private:

	// アニメーション
	void Animation();
};