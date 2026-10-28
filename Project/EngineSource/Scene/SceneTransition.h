#pragma once
#include"ITransitionEffect.h"
#include"Sprite.h"

class SceneTransition {
public:

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

public:

	/// <summary>
	/// トランジション開始
	/// </summary>
	/// <param name="type">演出タイプ</param>
	/// <param name="duration">演出時間（秒）</param>
	void Start(std::unique_ptr<ITransitionEffect> effect);

	/// <summary>
	/// トランジション中かどうか
	/// </summary>
	bool IsActive() const { return isActive_; }

	/// <summary>
	/// フェードアウトが完了したか
	/// </summary>
	bool IsMidTransition() const { return isActive_ && currentEffect_ && currentEffect_->IsMidTransition(timer_); }

private:
	// トランジションが有効か
	bool isActive_ = false;

	// 経過時間
	float timer_ = 0.0f;
	// 演出時間
	float maxTime_ = 1.0f;

	// 現在の演出オブジェクト
	std::unique_ptr<ITransitionEffect> currentEffect_;
};