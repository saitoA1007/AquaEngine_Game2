#include "pch.h"
#include "PlayerAnimator.h"
#include "FPSCounter.h"
using namespace GameEngine;

PlayerAnimator::PlayerAnimator(GameEngine::Model* model, GameEngine::AnimationManager* animationManager) {

	// アニメーションデータを取得
	animationData_[PlayerAnimationType::kWalk] = animationManager->GetNameByAnimations("PlayerWalk");
	animationData_[PlayerAnimationType::kAirMove] = animationManager->GetNameByAnimations("PlayerAirMove");
	animationData_[PlayerAnimationType::kRushAttack] = animationManager->GetNameByAnimations("PlayerRush");
	animationData_[PlayerAnimationType::kDownAttack] = animationManager->GetNameByAnimations("PlayerDownAttack");


	// アニメーターを初期化
	animator_.Initialize(model, &animationData_[PlayerAnimationType::kWalk]["歩き"]);
	currentType_ = PlayerAnimationType::kWalk;
}

void PlayerAnimator::Initialize() {
	// 初期設定
	StartAnimation(PlayerAnimationType::kWalk, "歩き");

	currentType_ = PlayerAnimationType::kWalk;
}

void PlayerAnimator::Update() {
	if (isStop_) { return; }

	timer_ += FpsCounter::gameDeltaTime / maxTime_;

	if (timer_ >= 1.0f) {

		if (isLoop_) {
			timer_ = 0.0f;
		} else {
			timer_ = 1.0f;
		}
	}

	animator_.ComputeUpdate(timer_);
}

void PlayerAnimator::StartAnimation(PlayerAnimationType type, const std::string& animeName, bool isLoop) {
	isLoop_ = isLoop;
	maxTime_ = animationData_[type][animeName].duration;
	timer_ = 0.0f;
	currentType_ = type;
	// アニメーションデータをセット
	animator_.SetAnimationData(&animationData_[type][animeName]);
}

void PlayerAnimator::StartAnimation(PlayerAnimationType type, const std::string& animeName, float maxTime, bool isLoop) {
	isLoop_ = isLoop;
	maxTime_ = maxTime;
	timer_ = 0.0f;
	currentType_ = type;
	// アニメーションデータをセット
	animator_.SetAnimationData(&animationData_[type][animeName]);
}