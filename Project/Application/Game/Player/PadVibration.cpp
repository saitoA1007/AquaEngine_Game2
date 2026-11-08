#include "pch.h"
#include "PadVibration.h"
#include "FPSCounter.h"
#include <algorithm>
#include <cmath>
using namespace GameEngine;

PadVibration::PadVibration(InputCommand* inputCommand) : inputCommand_(inputCommand) {
    RegisterDebugParameters();
}

PadVibration::~PadVibration() {
    Stop();
}

void PadVibration::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("PadVibration");
    debugParam_->Register("isEnabled", isEnabled_, 0);
    debugParam_->Register("chargeStrength", chargeStrength_, 1);
    debugParam_->Register("releaseStrength", releaseStrength_, 2);
    debugParam_->Register("hitStrength", hitStrength_, 3);
    debugParam_->Register("releaseTime", releaseTime_, 4);
    debugParam_->Register("hitTime", hitTime_, 5);
    debugParam_->Register("damageStrength", damageStrength_, 6);
    debugParam_->Register("damageTime", damageTime_, 7);
    debugParam_->Apply();
}

void PadVibration::Update() {
    debugParam_->ApplyIfDirty();

    // 短い振動の残り時間を進める
    float deltaTime = FpsCounter::GetGameDeltaTime();
    releaseTimer_ = (std::max)(releaseTimer_ - deltaTime, 0.0f);
    hitTimer_ = (std::max)(hitTimer_ - deltaTime, 0.0f);
    damageTimer_ = (std::max)(damageTimer_ - deltaTime, 0.0f);

    // 無効のときは何も出さない
    if (!isEnabled_) {
        Apply(0.0f);
        return;
    }

    // 続いている振動のうち、一番強いものを出す
    float strength = 0.0f;
    if (isCharging_) {
        strength = (std::max)(strength, chargeStrength_);
    }
    if (releaseTimer_ > 0.0f) {
        strength = (std::max)(strength, releaseStrength_);
    }
    if (hitTimer_ > 0.0f) {
        strength = (std::max)(strength, hitStrength_);
    }
    if (damageTimer_ > 0.0f) {
        strength = (std::max)(strength, damageStrength_);
    }
    Apply(strength);
}

void PadVibration::Stop() {
    isCharging_ = false;
    releaseTimer_ = 0.0f;
    hitTimer_ = 0.0f;
    damageTimer_ = 0.0f;
    Apply(0.0f);
}

void PadVibration::Apply(float strength) {
    // 値が変わっていなければ送らない（エンジンの振動は次に呼ぶまで続くため）
    if (std::abs(strength - lastStrength_) < 0.0001f) {
        return;
    }

    lastStrength_ = strength;
    inputCommand_->PlayPadVibration(strength, strength);
}
