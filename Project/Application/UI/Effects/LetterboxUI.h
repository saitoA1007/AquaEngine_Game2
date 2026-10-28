#pragma once
#include "IGameObject.h"
#include "Sprite.h"
#include "WorldTransform.h"
#include "DebugParameter.h"

/// <summary>
/// 画面を覆う上下の黒帯を表示
/// </summary>
class LetterboxUI : public GameEngine::IGameObject {
public:
	LetterboxUI(std::string name);
	~LetterboxUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 基準点に親を設定
	void SetParent(GameEngine::WorldTransform* parent) {
		baseWorld_.SetParent(parent);
	}

	// バー表示の有効設定
	void SetBarActive(bool isActive) {

		if (isBarActive_ != isActive) {
			timer_ = 0.0f;
		}

		isBarActive_ = isActive;
	}

private:

	float maxTime_ = 1.0f;

	float upBarStartPosY_;
	float upBarEndPosY_;

	float downBarStartPosY_;
	float downBarEndPosY_;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 基準の位置
	GameEngine::WorldTransform baseWorld_;

	// 上のバー
	GameEngine::Sprite upBarSprite_;

	// 下のバー
	GameEngine::Sprite downBarSprite_;

	float timer_ = 1.0f;

	bool isBarActive_ = false;

private:

	void BarAnimation();
};