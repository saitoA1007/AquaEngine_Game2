#include "pch.h"
#include"FPSCounter.h"
#include"ImGuiManager.h"
using namespace GameEngine;

float FpsCounter::maxFrameCount = 1;
float FpsCounter::deltaTime = 1.0f / 60.0f;
float FpsCounter::gameDeltaTime = 1.0f / 60.0f;
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
		maxFrameCount_ = frameCount_;
		elapsedTime_ = 0.0f;
		frameCount_ = 0;

		// 最大FPSを保存
		maxFrameCount = static_cast<float>(maxFrameCount_);
		// Δ時間を求める
		deltaTime = 1.0f / static_cast<float>(maxFrameCount_);
		gameDeltaTime = deltaTime;
	}
}