#include "pch.h"
#include "BossStateIn.h"
#include <numbers>
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

BossStateIn::BossStateIn(BossStateCommonData& commonData) : stateCommonData_(commonData) {

}

void BossStateIn::Enter() {
	// 初期位置
	stateCommonData_.worldTransform->transform_.translate = { 0.0f,0.0f,0.0f };
	stateCommonData_.worldTransform->transform_.rotate = { 0.0f,3.2f,0.0f };

	// アニメーション
	stateCommonData_.animator->StartAnimation(BossAnimationType::kAppearance, "Appearance_Animation_Rotate");

	// 初期化
	isMove_ = true;
	delayBreakEgg_ = true;
	timer_ = 0.0f;
}

void BossStateIn::Update() {

	if (!stateCommonData_.isBreakEgg) { return; }

	if (delayBreakEgg_) {
		timer_ += FpsCounter::gameDeltaTime / delayMaxTime_;

		if (timer_ >= 1.0f) {
			timer_ = 0.0f;
			delayBreakEgg_ = false;
			// 卵を描画しない
			stateCommonData_.isDrawEgg = false;
		}
	}

	if (delayBreakEgg_) { return; }

	if (isMove_) {
		timer_ += FpsCounter::gameDeltaTime / maxInTime_;

		// 上昇
		float riseY = Lerp(0.0f, moveHeight_, timer_);
		stateCommonData_.worldTransform->transform_.translate = startPos_;
		stateCommonData_.worldTransform->transform_.translate.y = startPos_.y + riseY;

		// 回転
		float targetDeg = Lerp(0.0f, 1620.0f, timer_);
		float targetRad = targetDeg * (std::numbers::pi_v<float> / 180.0f);
		stateCommonData_.worldTransform->transform_.rotate.y = targetRad;

		if (timer_ >= 1.0f) {
			stateCommonData_.worldTransform->transform_.rotate.y = 1620.0f * (std::numbers::pi_v<float> / 180.0f);
			isMove_ = false;
			// 次フェーズへ向けてタイマーリセット
			timer_ = 0.0f;

			// 叫びモージョンに以降
			stateCommonData_.animator->StartAnimation(BossAnimationType::kScream, "Scream", maxWaitTime_, false);

		}
	} else {
		// 待機タイマー
		timer_ += FpsCounter::gameDeltaTime / maxWaitTime_;

		if (timer_ >= 1.0f) {
			stateCommonData_.bossStateRequest = BossState::kBattle;

			// アニメーション遷移
			stateCommonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
		}
	}
}

void BossStateIn::Exit() {



}