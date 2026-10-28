#pragma once
#include <list>
#include "IGameObject.h"
#include "Sprite.h"
#include "WorldTransform.h"
#include "DebugParameter.h"

/// <summary>
/// バータイプのHP表示
/// </summary>
class HpBarUI : public GameEngine::IGameObject {
public:

	struct Point {
		float start;
		float end;
		float timer = 0.0f;
	};

public:
	HpBarUI(std::string name);
	~HpBarUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	void SetCurrentHp(int32_t hp) { currentHp_ = hp; }

	void SetMaxHp(int32_t hp) { maxHp_ = hp; }

	// 基準点に親を設定
	void SetParent(GameEngine::WorldTransform* parent) {
		baseWorld_.SetParent(parent);
	}

private:

	// バーの横サイズ
	float barSizeX_ = 100.0f;

	// バーが動く時間
	float effectmaxTime_ = 0.5f;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 基準の位置
	GameEngine::WorldTransform baseWorld_;

	// 体力
	GameEngine::Sprite barSprite_;

	// hpの減少演出用
	GameEngine::Sprite effectSprite_;

	// フレーム
	GameEngine::Sprite frameSprite_;

	// 最大hp
	int32_t maxHp_ = 1;

	// 現在のhp
	int32_t currentHp_ = 1;

	// 演出用のhpゲージが移動する位置
	std::list<Point> points_;

	float preScaleX_ = 1.0f;

private:

	/// <summary>
	/// 演出の更新処理
	/// </summary>
	void EffectUpdate();
};