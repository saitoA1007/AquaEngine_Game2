#include "pch.h"
#include "FieldScenery.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
using namespace GameEngine;

namespace {
    // 氷柱モデルの底の半径（RegisterCylinderModelでの登録値）。太さの倍率をかける
    constexpr float kPillarBaseRadius = 1.0f;
    // 雪原の最小の倍率（調整値が極端でも消えないようにする）
    constexpr float kMinSnowScale = 1.0f;
    // 度からラジアンへの変換
    constexpr float kDegreeToRadian = std::numbers::pi_v<float> / 180.0f;
    // 雪原の大きさの計算に使う半径の下限
    constexpr float kMinRadius = 1.0f;
}

FieldScenery::FieldScenery(const Field* field, Model* pillarModel, Model* planeModel)
    : field_(field), snowField_(planeModel) {

    // 上限の本数を最初に作っておく
    for (std::unique_ptr<ModelComponent>& pillar : pillars_) {
        pillar = std::make_unique<ModelComponent>(pillarModel);
        pillar->materialData_->enableLighting = 1;
    }
    snowField_.materialData_->enableLighting = 1;

    RegisterDebugParameters();
}

void FieldScenery::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("FieldScenery");
    debugParam_->Register("pillarCount", pillarCount_);
    debugParam_->Register("seed", seed_);
    debugParam_->Register("bandInner", bandInner_);
    debugParam_->Register("bandWidth", bandWidth_);
    debugParam_->Register("heightRange", heightRange_);
    debugParam_->Register("thicknessRange", thicknessRange_);
    debugParam_->Register("maxTiltDegree", maxTiltDegree_);
    debugParam_->Register("angleJitter", angleJitter_);
    debugParam_->Register("pillarColor", pillarColor_);
    debugParam_->Register("pillarRoughness", pillarRoughness_);
    debugParam_->Register("pillarMetallic", pillarMetallic_);
    debugParam_->Register("snowScale", snowScale_);
    debugParam_->Register("snowColor", snowColor_);
    debugParam_->Register("snowRoughness", snowRoughness_);
    debugParam_->Register("snowMetallic", snowMetallic_);
    debugParam_->Apply();
}

void FieldScenery::Initialize() {
    Update();
}

void FieldScenery::Update() {
    bool isChanged = debugParam_->ApplyIfDirty();

    // 台地の大きさが変わったときも並べ直す
    if (field_->GetRadius() != lastRadius_ ||
        field_->GetGroundThickness() != lastGroundThickness_ ||
        field_->GetRimWidth() != lastRimWidth_) {
        isChanged = true;
    }

    if (isChanged) {
        Rebuild();
    }

    UpdateSnowField();
    ApplyPillarMaterial();

    uint32_t count = std::min(pillarCount_, kMaxPillars);
    for (uint32_t i = 0; i < count; ++i) {
        pillars_[i]->Update();
    }
    snowField_.Update();
}

void FieldScenery::Draw() {
    snowField_.DrawRaytracing(renderQueue_);

    // 描く本数だけ登録する（使わない分は残したまま）
    uint32_t count = std::min(pillarCount_, kMaxPillars);
    for (uint32_t i = 0; i < count; ++i) {
        pillars_[i]->DrawRaytracing(renderQueue_);
    }
}

void FieldScenery::Rebuild() {
    lastRadius_ = field_->GetRadius();
    lastGroundThickness_ = field_->GetGroundThickness();
    lastRimWidth_ = field_->GetRimWidth();

    uint32_t count = std::min(pillarCount_, kMaxPillars);
    if (count == 0) {
        return;
    }

    // 範囲の入れ違いを直す
    float heightMin = std::fmin(heightRange_.x, heightRange_.y);
    float heightMax = std::fmax(heightRange_.x, heightRange_.y);
    float thicknessMin = std::fmin(thicknessRange_.x, thicknessRange_.y);
    float thicknessMax = std::fmax(thicknessRange_.x, thicknessRange_.y);

    // 並べるたびに同じ並びになるよう、シードから作り直す
    std::mt19937 random(seed_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    const float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    float angleStep = kTwoPi / static_cast<float>(count);
    float baseDistance = lastRadius_ + lastRimWidth_ + bandInner_;
    float maxTilt = maxTiltDegree_ * kDegreeToRadian;

    for (uint32_t i = 0; i < count; ++i) {
        // 角度は等間隔から間隔の割合だけずらす
        float angle = angleStep * static_cast<float>(i) + (unit(random) - 0.5f) * angleStep * angleJitter_;
        float distance = baseDistance + unit(random) * bandWidth_;
        float visibleHeight = heightMin + unit(random) * (heightMax - heightMin);
        float thickness = thicknessMin + unit(random) * (thicknessMax - thicknessMin);
        float tilt = unit(random) * maxTilt;
        float yaw = unit(random) * kTwoPi;

        float directionX = std::cos(angle);
        float directionZ = std::sin(angle);

        // 底は下の雪原（台地の底）に置き、台地の上に出る高さを visibleHeight にする
        float totalHeight = visibleHeight + lastGroundThickness_;
        Transform& transform = pillars_[i]->worldTransform_.transform_;
        transform.scale = { kPillarBaseRadius * thickness, totalHeight, kPillarBaseRadius * thickness };
        transform.translate = { directionX * distance, (visibleHeight - lastGroundThickness_) * 0.5f, directionZ * distance };

        // 外向きに傾ける（Xの回転で上端が+Z側、Zの回転で上端が-X側へ倒れる）
        transform.rotate = { tilt * directionZ, yaw, -tilt * directionX };
    }
}

void FieldScenery::UpdateSnowField() {
    float radius = std::fmax(field_->GetRadius(), kMinRadius);
    float scale = radius * std::fmax(snowScale_, kMinSnowScale);

    // 台地の底の高さに置く
    snowField_.worldTransform_.transform_.scale = { scale, 1.0f, scale };
    snowField_.worldTransform_.transform_.translate = { 0.0f, -field_->GetGroundThickness(), 0.0f };

    snowField_.materialData_->color = snowColor_;
    snowField_.materialData_->roughness = snowRoughness_;
    snowField_.materialData_->metallic = snowMetallic_;
}

void FieldScenery::ApplyPillarMaterial() {
    for (std::unique_ptr<ModelComponent>& pillar : pillars_) {
        pillar->materialData_->color = pillarColor_;
        pillar->materialData_->roughness = pillarRoughness_;
        pillar->materialData_->metallic = pillarMetallic_;
    }
}
