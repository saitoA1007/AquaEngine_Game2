#include "pch.h"
#include "Field.h"
#include <cmath>
using namespace GameEngine;

namespace {
    // 登録済みCylinderモデルの高さ（ResourceSubsystemでの登録値）。中心は原点。
    constexpr float kCylinderHeight = 3.0f;
}

Field::Field(Model* cylinderModel) : ground_(cylinderModel), rim_(cylinderModel) {

    RegisterDebugParameters();

    ground_.materialData_->enableLighting = 1;
    rim_.materialData_->enableLighting = 1;
}

void Field::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("Field");
    debugParam_->Register("radius", radius_);
    debugParam_->Register("rimWidth", rimWidth_);
    debugParam_->Register("groundThickness", groundThickness_);
    debugParam_->Register("rimThickness", rimThickness_);
    debugParam_->Register("groundColor", groundColor_);
    debugParam_->Register("rimColor", rimColor_);
    debugParam_->Register("groundRoughness", groundRoughness_);
    debugParam_->Register("groundMetallic", groundMetallic_);
    debugParam_->Register("rimRoughness", rimRoughness_);
    debugParam_->Register("rimMetallic", rimMetallic_);
    debugParam_->Apply();
}

void Field::Initialize() {
    Update();
}

void Field::Update() {
    debugParam_->ApplyIfDirty();

    // 半径が極端な値にならないようにする
    float radius = std::fmax(radius_, 1.0f);

    ground_.materialData_->color = groundColor_;
    rim_.materialData_->color = rimColor_;
    ground_.materialData_->roughness = groundRoughness_;
    ground_.materialData_->metallic = groundMetallic_;
    rim_.materialData_->roughness = rimRoughness_;
    rim_.materialData_->metallic = rimMetallic_;

    // 地面は上面がY=0になるように置く
    float groundScaleY = groundThickness_ / kCylinderHeight;
    ground_.worldTransform_.transform_.scale = { radius, groundScaleY, radius };
    ground_.worldTransform_.transform_.translate = { 0.0f, -groundThickness_ * 0.5f, 0.0f };

    // 縁は地面より少し大きく、上面を少し低くして地面の外周に見えるようにする
    float rimTop = -0.05f;
    rim_.worldTransform_.transform_.scale = { radius + rimWidth_, rimThickness_ / kCylinderHeight, radius + rimWidth_ };
    rim_.worldTransform_.transform_.translate = { 0.0f, rimTop - rimThickness_ * 0.5f, 0.0f };

    ground_.Update();
    rim_.Update();
}

void Field::Draw() {
    rim_.DrawRaytracing(renderQueue_);
    ground_.DrawRaytracing(renderQueue_);
}

Vector3 Field::ClampToField(const Vector3& position, float margin) const {
    float limit = std::fmax(radius_ - margin, 0.0f);
    float distance = std::sqrt(position.x * position.x + position.z * position.z);

    // 円内なら補正しない
    if (distance <= limit) {
        return position;
    }

    // 円周上へ戻す（distanceはlimit以上でありlimit>0のため0除算にならない。limit=0でも結果は原点）
    float scale = limit / distance;
    return { position.x * scale, position.y, position.z * scale };
}
