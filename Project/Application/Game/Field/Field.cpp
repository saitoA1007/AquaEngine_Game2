#include "pch.h"
#include "Field.h"
#include "Model.h"
#include "BufferRefManager.h"
#include "TLAS.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
using namespace GameEngine;

namespace {
    // 半径の下限（調整値が極端でも消えないようにする）
    constexpr float kMinRadius = 1.0f;
    // 氷の塊の高さ・幅の下限
    constexpr float kMinRockSize = 0.01f;
    // 氷の塊の個数の下限（円を囲むのに必要な最小の数）
    constexpr uint32_t kMinRocks = 3;
    // ひび割れの画像の1枚が覆う長さの下限（0除算を防ぐ）
    constexpr float kMinCrackTileSize = 0.01f;
    // 度からラジアンへの変換
    constexpr float kDegreeToRadian = std::numbers::pi_v<float> / 180.0f;
    // 氷の塊の種類を決める乱数のシード（種類は作るときに固定し、並べ直しでは変えない）
    constexpr uint32_t kRockKindSeed = 12345;
    // 氷の塊の氷マテリアルの初期値（結晶と同じ淡い水色）と粗さ
    const Vector4 kRockIceColor = { 0.55f, 0.78f, 0.95f, 1.0f };
    constexpr float kRockIceRoughness = 0.15f;
}

Field::Field(Model* discModel, const std::array<Model*, kRockKinds>& rockModels, uint32_t iceAlbedoTexture, uint32_t iceNormalTexture)
    : floor_(discModel) {

    floor_.materialData_->enableLighting = 1;
    // 床のひび割れの画像とノーマルマップ。繰り返しの倍率はUpdateFloorで決める
    floor_.materialData_->textureHandle = iceAlbedoTexture;
    floor_.materialData_->normalTextureHandle = iceNormalTexture;

    // 上限の個数を最初に作っておく。種類は乱数で決めて固定する
    std::mt19937 kindRandom(kRockKindSeed);
    std::uniform_int_distribution<uint32_t> kindDistribution(0, kRockKinds - 1);
    for (std::unique_ptr<ModelComponent>& rock : rocks_) {
        Model* model = rockModels[kindDistribution(kindRandom)];
        rock = std::make_unique<ModelComponent>(model);
        SetUpRockIce(*rock, model);
    }

    // 氷の塊は氷マテリアルで描く。初期値（色と粗さ）を決めてから調整値を登録する
    iceMaterial_.GetMaterialData()->color = kRockIceColor;
    iceMaterial_.GetMaterialData()->roughness = kRockIceRoughness;

    RegisterDebugParameters();
    RegisterRockIceParameters();
}

void Field::SetUpRockIce(ModelComponent& rock, const Model* model) {
    // 設定はメッシュごとなので、モデルのメッシュ数だけ繰り返す
    uint32_t meshCount = static_cast<uint32_t>(model->GetMeshes().size());
    for (uint32_t mesh = 0; mesh < meshCount; ++mesh) {
        rock.SetBufferMaterial(static_cast<uint32_t>(BufferType::kDefalutMaterial), iceMaterial_.GetMaterialSrvIndex(), mesh);
        rock.SetHitGroup(1, mesh);
        rock.SetInstanceMask(static_cast<uint32_t>(RayInstanceMask::kRayMaskIce), mesh);
    }
}

void Field::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("Field");
    debugParam_->Register("radius", radius_);
    debugParam_->Register("rockCount", rockCount_);
    debugParam_->Register("rockSeed", rockSeed_);
    debugParam_->Register("rockHeightRange", rockHeightRange_);
    debugParam_->Register("rockWidthRange", rockWidthRange_);
    debugParam_->Register("rockRadialJitter", rockRadialJitter_);
    debugParam_->Register("rockMaxTiltDegree", rockMaxTiltDegree_);
    debugParam_->Register("rockEmbedRatio", rockEmbedRatio_);
    debugParam_->Register("rockInset", rockInset_);
    debugParam_->Register("floorColor", floorColor_);
    debugParam_->Register("floorRoughness", floorRoughness_);
    debugParam_->Register("floorMetallic", floorMetallic_);
    debugParam_->Register("crackTileSize", crackTileSize_);
    debugParam_->Apply();
}

void Field::RegisterRockIceParameters() {
    IceMaterial::IceMaterialData* ice = iceMaterial_.GetMaterialData();
    const std::string subGroup = "RockIce";
    int index = 0;
    debugParam_->Register("color", ice->color, index++, subGroup);
    debugParam_->Register("roughness", ice->roughness, index++, subGroup);
    debugParam_->Register("ior", ice->ior, index++, subGroup);
    debugParam_->Register("chipScale", ice->chipScale, index++, subGroup);
    debugParam_->Register("chipStrength", ice->chipStrength, index++, subGroup);
    debugParam_->Register("edgeWidth", ice->edgeWidth, index++, subGroup);
    debugParam_->Register("edgeStrength", ice->edgeStrength, index++, subGroup);
    debugParam_->Register("microScale", ice->microScale, index++, subGroup);
    debugParam_->Register("microStrength", ice->microStrength, index++, subGroup);
    debugParam_->Register("bubbleScale", ice->bubbleScale, index++, subGroup);
    debugParam_->Register("bubbleMaxDepth", ice->bubbleMaxDepth, index++, subGroup);
    debugParam_->Register("bubbleDensity", ice->bubbleDensity, index++, subGroup);
    debugParam_->Register("bubbleJitter", ice->bubbleJitter, index++, subGroup);
    debugParam_->Register("bubbleHighlight", ice->bubbleHighlight, index++, subGroup);
    debugParam_->Register("rimIntensity", ice->rimIntensity, index++, subGroup);
    debugParam_->Register("rimPower", ice->rimPower, index++, subGroup);
    debugParam_->Register("rimColor", ice->rimColor, index++, subGroup);
    debugParam_->Apply();
}

void Field::Initialize() {
    // 初回は必ず並べる
    lastRadius_ = -1.0f;
    Update();
}

uint32_t Field::GetRockCount() const {
    return std::clamp(rockCount_, kMinRocks, kMaxRocks);
}

float Field::GetWallWidth() const {
    // 氷の塊の外側の端（最大の幅と半径方向のずれ）。円の内側への食い込みは差し引く
    float maxWidth = std::fmax(std::fmax(rockWidthRange_.x, rockWidthRange_.y), kMinRockSize);
    return std::fmax(maxWidth - rockInset_ + std::fmax(rockRadialJitter_, 0.0f), kMinRockSize);
}

void Field::Update() {
    bool isChanged = debugParam_->ApplyIfDirty();

    UpdateFloor();

    // 調整値が変わったときだけ、氷の塊を並べ直す
    if (isChanged || radius_ != lastRadius_) {
        RebuildRocks();
    }
    // 氷の塊の行列はRebuildRocksで作ったものをそのまま使う（Updateで上書きしない）
}

void Field::UpdateFloor() {
    float radius = std::fmax(radius_, kMinRadius);

    // 円盤は半径1のXZ平面なので、半径をXとZに掛ける
    floor_.worldTransform_.transform_.scale = { radius, 1.0f, radius };
    floor_.worldTransform_.transform_.translate = { 0.0f, 0.0f, 0.0f };

    // 画像の繰り返し回数は、直径÷1枚が覆う長さ。半径を変えても、ひびの細かさは変わらない
    float tileCount = radius * 2.0f / std::fmax(crackTileSize_, kMinCrackTileSize);
    floor_.materialData_->uvTransform = Matrix4x4::MakeScaleMatrix({ tileCount, tileCount, 1.0f });

    floor_.materialData_->color = floorColor_;
    floor_.materialData_->roughness = floorRoughness_;
    floor_.materialData_->metallic = floorMetallic_;
    floor_.Update();
}

void Field::RebuildRocks() {
    lastRadius_ = radius_;

    float radius = std::fmax(radius_, kMinRadius);
    uint32_t count = GetRockCount();

    // 範囲の入れ違いを直す
    float heightMin = std::fmax(std::fmin(rockHeightRange_.x, rockHeightRange_.y), kMinRockSize);
    float heightMax = std::fmax(std::fmax(rockHeightRange_.x, rockHeightRange_.y), kMinRockSize);
    float widthMin = std::fmax(std::fmin(rockWidthRange_.x, rockWidthRange_.y), kMinRockSize);
    float widthMax = std::fmax(std::fmax(rockWidthRange_.x, rockWidthRange_.y), kMinRockSize);
    float maxTilt = rockMaxTiltDegree_ * kDegreeToRadian;

    // 並べるたびに同じ並びになるよう、シードから作り直す
    std::mt19937 random(rockSeed_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    const float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    float angleStep = kTwoPi / static_cast<float>(count);

    for (uint32_t i = 0; i < count; ++i) {
        float width = widthMin + unit(random) * (widthMax - widthMin);
        float height = heightMin + unit(random) * (heightMax - heightMin);

        // 角度は等間隔から少しずらし、半径は円周の外側へ半分の幅だけ張り出した所を中心にする
        float angle = angleStep * (static_cast<float>(i) + (unit(random) - 0.5f) * 0.5f);
        float centerRadius = radius + width * 0.5f - rockInset_ + (unit(random) * 2.0f - 1.0f) * rockRadialJitter_;
        Vector3 position = {
            std::cos(angle) * centerRadius,
            -height * rockEmbedRatio_,   // 底をわずかに埋めて、床との縁が浮かないようにする
            std::sin(angle) * centerRadius };

        float yaw = unit(random) * kTwoPi;
        // 傾ける軸は水平方向のランダムな向き、角度は0から最大まで
        float tiltDirection = unit(random) * kTwoPi;
        float tilt = unit(random) * maxTilt;
        Vector3 tiltAxis = { std::cos(tiltDirection), 0.0f, std::sin(tiltDirection) };

        // 拡縮、向き（Y回転）、傾き、移動の順に行列で合成する
        Matrix4x4 world = Matrix4x4::MakeScaleMatrix({ width, height, width })
            * Matrix4x4::MakeRotateYMatrix(yaw)
            * Matrix4x4::MakeRotateAxisAngle(tiltAxis, tilt)
            * Matrix4x4::MakeTranslateMatrix(position);
        rocks_[i]->worldTransform_.UpdateWorldMatrix(world);
    }
}

void Field::Draw() {
    floor_.DrawRaytracing(renderQueue_);

    // 描く個数だけ登録する（使わない分は残したまま）
    uint32_t count = GetRockCount();
    for (uint32_t i = 0; i < count; ++i) {
        rocks_[i]->DrawRaytracing(renderQueue_);
    }
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
