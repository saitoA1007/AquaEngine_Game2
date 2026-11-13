#include "pch.h"
#include "PlayerView.h"
#include "Matrix4x4.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
using namespace GameEngine;

PlayerView::PlayerView(const PlayerViewAssets& assets)
    : core_(RequireModel(assets.core)), frameA_(RequireModel(assets.frame)), frameB_(RequireModel(assets.frame)) {

    RegisterDebugParameters();

    // 核と枠は光を使う通常の材質で描く（氷の材質は使わない）
    core_.materialData_->enableLighting = 1;
    frameA_.materialData_->enableLighting = 1;
    frameB_.materialData_->enableLighting = 1;
    ApplyColors();
}

Model* PlayerView::RequireModel(Model* model) {
    if (model == nullptr) {
        throw std::runtime_error("PlayerView: core or frame model is missing");
    }
    return model;
}

void PlayerView::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>(kDebugName);

    // 大きさ・位置
    const std::string kLayoutGroup = "Layout";
    debugParam_->Register("viewScale", viewScale_, 0, kLayoutGroup);
    debugParam_->Register("liftHeight", liftHeight_, 1, kLayoutGroup);
    debugParam_->Register("frameScaleA", frameScaleA_, 2, kLayoutGroup);
    debugParam_->Register("frameScaleB", frameScaleB_, 3, kLayoutGroup);

    // 回転
    const std::string kSpinGroup = "Spin";
    debugParam_->Register("spinSpeedA", spinSpeedA_, 0, kSpinGroup);
    debugParam_->Register("spinSpeedB", spinSpeedB_, 1, kSpinGroup);
    debugParam_->Register("minChargeSpinScale", minChargeSpinScale_, 2, kSpinGroup);
    debugParam_->Register("chainSpinScale", chainSpinScale_, 3, kSpinGroup);
    debugParam_->Register("spinFollowSpeed", spinFollowSpeed_, 4, kSpinGroup);

    // 氷霧の拡縮
    const std::string kPulseGroup = "Pulse";
    debugParam_->Register("pulseScale", pulseScale_, 0, kPulseGroup);
    debugParam_->Register("pulseTime", pulseTime_, 1, kPulseGroup);

    // 色
    const std::string kColorGroup = "Color";
    debugParam_->Register("coreColor", coreColor_, 0, kColorGroup);
    debugParam_->Register("frameColor", frameColor_, 1, kColorGroup);

    debugParam_->Apply();
}

void PlayerView::Update(float deltaTime, float chargeRatio, bool isChainActive) {
    debugParam_->ApplyIfDirty();
    ApplyColors();

    // 回転の速さを目標へ近づけてから、枠を回す
    UpdateSpinSpeed(deltaTime, chargeRatio, isChainActive);
    angleA_ = std::fmod(angleA_ + spinSpeedA_ * spinScale_ * deltaTime, kTwoPi);
    angleB_ = std::fmod(angleB_ + spinSpeedB_ * spinScale_ * deltaTime, kTwoPi);

    UpdatePulse(deltaTime);
}

void PlayerView::UpdateSpinSpeed(float deltaTime, float chargeRatio, bool isChainActive) {
    // チャージが溜まるほど遅くなる。連鎖中は速くなり、重なったら掛け合わせる
    float ratio = std::clamp(chargeRatio, 0.0f, 1.0f);
    float targetScale = 1.0f + (minChargeSpinScale_ - 1.0f) * ratio;
    if (isChainActive) {
        targetScale *= chainSpinScale_;
    }

    // フレームレートによらず同じ速さで追いつくよう、指数で近づける
    float follow = 1.0f - std::exp(-(std::max)(spinFollowSpeed_, 0.0f) * deltaTime);
    spinScale_ += (targetScale - spinScale_) * follow;
}

void PlayerView::PlayPulse() {
    pulseTimer_ = (std::max)(pulseTime_, 0.01f);
}

void PlayerView::UpdatePulse(float deltaTime) {
    // 出ていなければ元の大きさ
    if (pulseTimer_ <= 0.0f) {
        pulseMultiplier_ = 1.0f;
        return;
    }

    // 1 → pulseScale → 1 を、sinの山で滑らかにつなぐ
    pulseTimer_ = (std::max)(pulseTimer_ - deltaTime, 0.0f);
    float time = (std::max)(pulseTime_, 0.01f);
    float progress = 1.0f - pulseTimer_ / time;
    pulseMultiplier_ = 1.0f + (pulseScale_ - 1.0f) * std::sin(kPi * progress);
}

void PlayerView::ApplyColors() {
    core_.materialData_->color = coreColor_;
    frameA_.materialData_->color = frameColor_;
    frameB_.materialData_->color = frameColor_;
}

void PlayerView::ApplyTransforms(const Vector3& position) {
    // 描く高さだけを上げる。位置そのものは変えない
    Vector3 center = { position.x, position.y + liftHeight_, position.z };
    Matrix4x4 move = Matrix4x4::MakeTranslateMatrix(center);
    float baseScale = viewScale_ * pulseMultiplier_;

    // 核は回さず、揺らさない
    core_.worldTransform_.SetWorldMatrix(Matrix4x4::MakeScaleMatrix({ baseScale, baseScale, baseScale }) * move);

    // 枠は水平に寝かせてから、AはX軸、BはZ軸のまわりに回す（行列は左から順に適用される）
    Matrix4x4 flat = Matrix4x4::MakeRotateXMatrix(kFlatAngle);
    float scaleA = baseScale * frameScaleA_;
    float scaleB = baseScale * frameScaleB_;
    frameA_.worldTransform_.SetWorldMatrix(
        Matrix4x4::MakeScaleMatrix({ scaleA, scaleA, scaleA }) * flat * Matrix4x4::MakeRotateXMatrix(angleA_) * move);
    frameB_.worldTransform_.SetWorldMatrix(
        Matrix4x4::MakeScaleMatrix({ scaleB, scaleB, scaleB }) * flat * Matrix4x4::MakeRotateZMatrix(angleB_) * move);
}

void PlayerView::Draw(RenderQueue* renderQueue, const Vector3& position) {
    ApplyTransforms(position);

    core_.DrawRaytracing(renderQueue);
    frameA_.DrawRaytracing(renderQueue);
    frameB_.DrawRaytracing(renderQueue);
}
