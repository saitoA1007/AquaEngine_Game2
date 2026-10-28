#pragma once
#include "Application/Enemy/IBossState.h"

class BossStateOut : public IBossState {
public:

	enum class Phase {
		kIn,
		kFade
	};

public:
	BossStateOut(BossStateCommonData& commonData);
	~BossStateOut() = default;

	/// <summary>
	/// 入りの処理
	/// </summary>
	void Enter() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 終わりの処理
	/// </summary>
	void Exit() override;

private:
	BossStateCommonData& stateCommonData_;

	uint32_t cout_ = 0;

	float timer_ = 0.0f;
	float InmaxTime_ = 4.0f;
	float FadeMaxTime_ = 2.0f;

	bool isSet_ = false;
	bool isActive_ = false;

	// 高さ
	float startPosY_ = 0.0f;
	float endPosY_ = 1.0f;

	// 左右の触れば
	float swaySpeed_ = 0.0f;
	float swayPhase_ = 0.0f;
	float swayWeithX_ = 10.0f;
	float swayWeithZ_ = 5.0f;
	float cycleHeight_ = 2.0f;

	Phase phase_ = Phase::kIn;
};