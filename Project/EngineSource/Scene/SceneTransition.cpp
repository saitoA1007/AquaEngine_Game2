#include "pch.h"
#include"SceneTransition.h"
#include"EasingManager.h"
#include"FPSCounter.h"
using namespace GameEngine;

void SceneTransition::Initialize() {

}

void SceneTransition::Update() {
	if (!isActive_ || !currentEffect_) {
		return;
	}

	timer_ += FpsCounter::deltaTime / maxTime_;

	// 演出の更新処理
	currentEffect_->Update(timer_);

	// 演出の終了
	if (timer_ >= 1.0f) {
		isActive_ = false;
		currentEffect_.reset();
	}
}

void SceneTransition::Draw() {
	if (currentEffect_) {
		currentEffect_->Draw();
	}
}

void SceneTransition::Start(std::unique_ptr<ITransitionEffect> effect) {
	currentEffect_ = std::move(effect);
	currentEffect_->Initialize();

	isActive_ = true;
	maxTime_ = currentEffect_->GetMaxTime();
	timer_ = 0.0f;
}