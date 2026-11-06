#include "pch.h"
#include"FPSCounter.h"
#include"ImGuiManager.h"
#include <algorithm>
using namespace GameEngine;

float FpsCounter::maxFrameCount_ = 1.0f;
float FpsCounter::deltaTime_ = 1.0f / 60.0f;
float FpsCounter::gameDeltaTime_ = 1.0f / 60.0f;
float FpsCounter::timeScale_ = 1.0f;
int FpsCounter::frameCount_ = 0;

void FpsCounter::Initialize() {
	preTime_ = std::chrono::high_resolution_clock::now();
}

void FpsCounter::Update() {
	// 現在時刻を取得
	currentTime_ = std::chrono::high_resolution_clock::now();

	// 前のフレームとの経過時間を求める
	std::chrono::duration<float> delta = currentTime_ - preTime_;
	preTime_ = currentTime_;

	// 秒数を取得
	elapsedTime_ += delta.count();
	frameCount_++;

	// 1秒ごとにFPSを更新
	if (elapsedTime_ >= 1.0f) {
		// 最大FPSを保存
		maxFrameCount_ = static_cast<float>(frameCount_);
		elapsedTime_ = 0.0f;
		frameCount_ = 0;

		// Δ時間を求める
		deltaTime_ = 1.0f / maxFrameCount_;
	}

	// ゲーム用のデルタ時間はタイムスケールを反映して毎フレーム更新する
	gameDeltaTime_ = deltaTime_ * timeScale_;
}

void FpsCounter::SetTimeScale(float timeScale) {
	timeScale_ = std::max(timeScale, 0.0f);
	// このフレームの途中で設定された場合もすぐに反映する
	gameDeltaTime_ = deltaTime_ * timeScale_;
}
