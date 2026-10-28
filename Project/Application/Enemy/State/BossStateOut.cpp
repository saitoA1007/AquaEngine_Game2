#include "pch.h"
#include "BossStateOut.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

BossStateOut::BossStateOut(BossStateCommonData& commonData) : stateCommonData_(commonData) {

	std::string subGroup = "StateOutAnimation";
	int index = 0;
	stateCommonData_.debugParame->Register("InMaxTime", InmaxTime_, index++, subGroup);
	stateCommonData_.debugParame->Register("FadeMaxTime", FadeMaxTime_, index++, subGroup);
	stateCommonData_.debugParame->Register("StartPosY", startPosY_, index++, subGroup);
	stateCommonData_.debugParame->Register("EndPosY", endPosY_, index++, subGroup);
	stateCommonData_.debugParame->Register("SwaySpeed", swaySpeed_, index++, subGroup);
	stateCommonData_.debugParame->Register("SwayPhase", swayPhase_, index++, subGroup);
	stateCommonData_.debugParame->Register("SwayWeithX", swayWeithX_, index++, subGroup);
	stateCommonData_.debugParame->Register("SwayWeithZ", swayWeithZ_, index++, subGroup);
	stateCommonData_.debugParame->Register("CycleHeight", cycleHeight_, index++, subGroup);
}

void BossStateOut::Enter() {

	// 初期位置
	stateCommonData_.worldTransform->transform_.translate = { 0.0f,2.0f,0.0f };
	stateCommonData_.worldTransform->transform_.rotate = { 0.0f,3.2f,0.0f };

	phase_ = Phase::kIn;
	timer_ = 0.0f;
}

void BossStateOut::Update() {

	switch (phase_)
	{
	case BossStateOut::Phase::kIn: {
		timer_ += FpsCounter::gameDeltaTime / InmaxTime_;

		float swayFade = 1.0f - timer_;

		float timeValue = timer_ * swaySpeed_ + swayPhase_;
		float swayOffsetX = std::sinf(timeValue) * swayWeithX_ * swayFade;
		float swayOffsetZ = std::sinf(timeValue * 2.0f) * swayWeithZ_ * swayFade;

		Vector3 basePos;
		basePos.x = 0.0f;
		basePos.y = Lerp(startPosY_, endPosY_, timer_, EaseType::kEaseInOutQuad);
		basePos.z = 0.0f;

		// 縦移動
		float posY = 0.0f;
		float totalCycle = timer_ * 3.0f;
		float localTimer = std::fmodf(totalCycle, 1.0f);

		if (localTimer <= 0.5f) {
			float t = localTimer / 0.5f;
			posY = Lerp(0.0f, cycleHeight_, t, EaseType::kEaseInOutQuad);
		} else {
			float t = (localTimer - 0.5f) / 0.5f;
			posY = Lerp(cycleHeight_, 0.0f, t, EaseType::kEaseInOutQuad);
		}

		basePos.y += posY;

		stateCommonData_.worldTransform->transform_.translate = basePos + Vector3(swayOffsetX, 0.0f, swayOffsetZ);

		if (timer_ >= 1.0f) {
			phase_ = Phase::kFade;
			timer_ = 0.0f;
		}
		break;
	}

	case BossStateOut::Phase::kFade: {

		timer_ += FpsCounter::gameDeltaTime / FadeMaxTime_;

		if (timer_ >= 1.0f) {
			// ボスの全ての処理が終了
			isActive_ = false;
		}
		break;
	}
	}
}

void BossStateOut::Exit() {

}