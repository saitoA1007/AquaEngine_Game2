#include "pch.h"
#include "GameCamera.h"
#include "FPSCounter.h"
#include <cmath>
#include <numbers>
using namespace GameEngine;

GameCamera::GameCamera(int clientWidth, int clientHeight) {
    camera_ = std::make_unique<Camera>();
    camera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, clientWidth, clientHeight);

    RegisterDebugParameters();
}

void GameCamera::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("GameCamera");
    debugParam_->Register("distance", distance_);
    debugParam_->Register("pitchDeg", pitchDeg_);
    debugParam_->Register("followRate", followRate_);
    debugParam_->Apply();
}

void GameCamera::Update(const Vector3& targetPosition) {
    debugParam_->ApplyIfDirty();

    // 目標位置 = ターゲット + (0, 距離×sin(俯角), -距離×cos(俯角))
    float pitch = pitchDeg_ * std::numbers::pi_v<float> / 180.0f;
    Vector3 offset = { 0.0f, distance_ * std::sin(pitch), -distance_ * std::cos(pitch) };
    Vector3 target = targetPosition + offset;

    // 初回は補間せず目標位置へ合わせる
    if (!isInitialized_) {
        position_ = target;
        isInitialized_ = true;
    }

    // 指数補間。フレームレートに依存せず、毎秒 followRate の割合で差が縮む
    float t = 1.0f - std::exp(-followRate_ * FpsCounter::GetGameDeltaTime());
    position_ += (target - position_) * t;

    // 固定角度。X回転の正は下向き
    camera_->transform_.rotate = { pitch, 0.0f, 0.0f };
    camera_->transform_.translate = position_;
    camera_->Update();
}
