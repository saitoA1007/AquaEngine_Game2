#include "pch.h"
#include "BossAnimator.h"
#include "FPSCounter.h"
using namespace GameEngine;

BossAnimator::BossAnimator(GameEngine::Model* model, GameEngine::AnimationManager* animationManager) {

	// アニメーションデータを取得
	animationData_[BossAnimationType::kMove] = animationManager->GetNameByAnimations("BossBirdBaseMove");
	animationData_[BossAnimationType::kBreath] = animationManager->GetNameByAnimations("BossBirdIceBreath");
	animationData_[BossAnimationType::kRush] = animationManager->GetNameByAnimations("BossBirdRush");
	animationData_[BossAnimationType::kScream] = animationManager->GetNameByAnimations("BossBirdScream");
	animationData_[BossAnimationType::kAppearance] = animationManager->GetNameByAnimations("BossBirdAppearance");
	animationData_[BossAnimationType::kDeath] = animationManager->GetNameByAnimations("BossBirdShootDown");

	// アニメーターを初期化
	animator_.Initialize(model, &animationData_[BossAnimationType::kMove]["基本移動"]);
	currentType_ = BossAnimationType::kMove;
}

void BossAnimator::Initialize() {
	// 初期設定
	StartAnimation(BossAnimationType::kMove, "基本移動");
	currentType_ = BossAnimationType::kMove;
}

void BossAnimator::Update() {
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

void BossAnimator::StartAnimation(BossAnimationType type, const std::string& animeName, bool isLoop) {
	isLoop_ = isLoop;
	maxTime_ = animationData_[type][animeName].duration;
	timer_ = 0.0f;
	currentType_ = type;
	// アニメーションデータをセット
	animator_.SetAnimationData(&animationData_[type][animeName]);
}

void BossAnimator::StartAnimation(BossAnimationType type, const std::string& animeName, float maxTime, bool isLoop) {
	isLoop_ = isLoop;
	maxTime_ = maxTime;
	timer_ = 0.0f;
	currentType_ = type;
	// アニメーションデータをセット
	animator_.SetAnimationData(&animationData_[type][animeName]);
}