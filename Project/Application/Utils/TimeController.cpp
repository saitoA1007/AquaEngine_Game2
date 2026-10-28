#include "pch.h"
#include "TimeController.h"
#include "FPSCounter.h"
using namespace GameEngine;

TimeController::TimeController() {

}

void TimeController::Update() {

	// 時間を止める
	if (isStopTimeActive_) {

		timer_ += FpsCounter::deltaTime / stopMaxTime_;

		// デルタタイムを0にして止める
		FpsCounter::gameDeltaTime = 0.0f;

		if (timer_ >= 1.0f) {
			timer_ = 0.0f;
			isStopTimeActive_ = false;
		}
	}
}