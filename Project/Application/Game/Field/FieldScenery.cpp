#include "pch.h"
#include "FieldScenery.h"
#include "Model.h"
#include "BufferRefManager.h"
#include "TLAS.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
using namespace GameEngine;

namespace {
    // 雪原の最小の倍率（調整値が極端でも消えないようにする）
    constexpr float kMinSnowScale = 1.0f;
    // 度からラジアンへの変換
    constexpr float kDegreeToRadian = std::numbers::pi_v<float> / 180.0f;
    // 雪原の大きさの計算に使う半径の下限
    constexpr float kMinRadius = 1.0f;
    // 位置から向きを求めるときの、ゼロ除算を避ける長さの下限
    constexpr float kMinDirectionLength = 0.0001f;
    // 雪の画像の1枚が覆う長さの下限（0除算を防ぐ）
    constexpr float kMinSnowTileSize = 0.01f;
    // 結晶の氷の色の初期値（淡い水色）と粗さの初期値
    const Vector4 kCrystalIceColor = { 0.55f, 0.78f, 0.95f, 1.0f };
    constexpr float kCrystalIceRoughness = 0.15f;
}

FieldScenery::FieldScenery(const Field* field, const std::array<Model*, kCrystalKinds>& crystalModels, Model* planeModel,
    uint32_t snowAlbedoTexture, uint32_t snowNormalTexture)
    : field_(field), snowField_(planeModel) {

    // 種類ごとに上限の数を最初に作っておく
    for (uint32_t kind = 0; kind < kCrystalKinds; ++kind) {
        for (std::unique_ptr<ModelComponent>& crystal : crystals_[kind]) {
            crystal = std::make_unique<ModelComponent>(crystalModels[kind]);
        }
    }
    snowField_.materialData_->enableLighting = 1;
    // 外の雪原の雪の画像とノーマルマップ。繰り返しの倍率はUpdateSnowFieldで決める
    snowField_.materialData_->textureHandle = snowAlbedoTexture;
    snowField_.materialData_->normalTextureHandle = snowNormalTexture;

    // 結晶は氷マテリアルで描く。初期値（色と粗さ）を決めてから調整値を登録する
    iceMaterial_.GetMaterialData()->color = kCrystalIceColor;
    iceMaterial_.GetMaterialData()->roughness = kCrystalIceRoughness;
    SetUpCrystalIce(crystalModels);

    RegisterDebugParameters();
    RegisterCrystalIceParameters();
}

void FieldScenery::SetUpCrystalIce(const std::array<Model*, kCrystalKinds>& crystalModels) {
    for (uint32_t kind = 0; kind < kCrystalKinds; ++kind) {
        // 設定はメッシュごとなので、モデルのメッシュ数だけ繰り返す
        uint32_t meshCount = static_cast<uint32_t>(crystalModels[kind]->GetMeshes().size());
        for (std::unique_ptr<ModelComponent>& crystal : crystals_[kind]) {
            for (uint32_t mesh = 0; mesh < meshCount; ++mesh) {
                crystal->SetBufferMaterial(static_cast<uint32_t>(BufferType::kDefalutMaterial), iceMaterial_.GetMaterialSrvIndex(), mesh);
                crystal->SetHitGroup(1, mesh);
                crystal->SetInstanceMask(static_cast<uint32_t>(RayInstanceMask::kRayMaskIce), mesh);
            }
        }
    }
}

void FieldScenery::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("FieldScenery");
    debugParam_->Register("clusterCount", clusterCount_);
    debugParam_->Register("crystalsPerCluster", crystalsPerCluster_);
    debugParam_->Register("seed", seed_);
    debugParam_->Register("bandInner", bandInner_);
    debugParam_->Register("bandWidth", bandWidth_);
    debugParam_->Register("clusterSpread", clusterSpread_);
    debugParam_->Register("angleJitter", angleJitter_);
    debugParam_->Register("heightRange", heightRange_);
    debugParam_->Register("thicknessScale", thicknessScale_);
    debugParam_->Register("maxTiltDegree", maxTiltDegree_);
    debugParam_->Register("occlusionRatio", occlusionRatio_);
    debugParam_->Register("snowScale", snowScale_);
    debugParam_->Register("snowOffsetY", snowOffsetY_);
    debugParam_->Register("snowColor", snowColor_);
    debugParam_->Register("snowRoughness", snowRoughness_);
    debugParam_->Register("snowMetallic", snowMetallic_);
    debugParam_->Register("snowTileSize", snowTileSize_);
    debugParam_->Apply();
}

void FieldScenery::RegisterCrystalIceParameters() {
    IceMaterial::IceMaterialData* ice = iceMaterial_.GetMaterialData();
    const std::string subGroup = "CrystalIce";
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

void FieldScenery::Initialize() {
    Update();
}

void FieldScenery::Update() {
    bool isChanged = debugParam_->ApplyIfDirty();

    // フィールドの半径や壁の幅が変わったときも並べ直す
    if (field_->GetRadius() != lastRadius_ || field_->GetWallWidth() != lastWallWidth_) {
        isChanged = true;
    }

    if (isChanged) {
        Rebuild();
    }

    UpdateSnowField();
    snowField_.Update();
    // 結晶の行列はRebuildで作ったものをそのまま使う（Updateで上書きしない）
}

void FieldScenery::Draw() {
    snowField_.DrawRaytracing(renderQueue_);

    // 使う数だけ登録する（使わない分は残したまま）
    for (uint32_t kind = 0; kind < kCrystalKinds; ++kind) {
        for (uint32_t i = 0; i < usedCounts_[kind]; ++i) {
            crystals_[kind][i]->DrawRaytracing(renderQueue_);
        }
    }
}

void FieldScenery::Rebuild() {
    lastRadius_ = field_->GetRadius();
    lastWallWidth_ = field_->GetWallWidth();
    usedCounts_.fill(0);

    uint32_t clusterCount = std::min(clusterCount_, kMaxClusters);
    if (clusterCount == 0) {
        return;
    }

    // 範囲の入れ違いを直す
    float heightMin = std::fmin(heightRange_.x, heightRange_.y);
    float heightMax = std::fmax(heightRange_.x, heightRange_.y);
    uint32_t perClusterMin = std::clamp(static_cast<uint32_t>(std::fmax(std::fmin(crystalsPerCluster_.x, crystalsPerCluster_.y), 1.0f)), 1u, kMaxCrystalsPerCluster);
    uint32_t perClusterMax = std::clamp(static_cast<uint32_t>(std::fmax(std::fmax(crystalsPerCluster_.x, crystalsPerCluster_.y), 1.0f)), perClusterMin, kMaxCrystalsPerCluster);

    // 並べるたびに同じ並びになるよう、シードから作り直す
    std::mt19937 random(seed_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_int_distribution<uint32_t> kindDistribution(0, kCrystalKinds - 1);
    std::uniform_int_distribution<uint32_t> countDistribution(perClusterMin, perClusterMax);

    const float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    float angleStep = kTwoPi / static_cast<float>(clusterCount);
    float wallOuterRadius = lastRadius_ + lastWallWidth_;
    float maxTilt = maxTiltDegree_ * kDegreeToRadian;

    for (uint32_t cluster = 0; cluster < clusterCount; ++cluster) {
        // 群れの角度は等間隔から間隔の割合だけずらす
        float clusterAngle = angleStep * static_cast<float>(cluster) + (unit(random) - 0.5f) * angleStep * angleJitter_;
        Vector3 radial = { std::cos(clusterAngle), 0.0f, std::sin(clusterAngle) };
        Vector3 tangent = { -radial.z, 0.0f, radial.x };

        uint32_t crystalCount = countDistribution(random);
        for (uint32_t n = 0; n < crystalCount; ++n) {
            // 種類は4種からランダム。上限に達した種類は次の種類へ回す
            uint32_t kind = kindDistribution(random);
            for (uint32_t tried = 0; tried < kCrystalKinds && usedCounts_[kind] >= kMaxCrystalsPerKind; ++tried) {
                kind = (kind + 1) % kCrystalKinds;
            }
            if (usedCounts_[kind] >= kMaxCrystalsPerKind) {
                continue;
            }

            // 壁の外側からの距離と、群れの中心からの横のずれで位置を決める
            float distance = bandInner_ + unit(random) * bandWidth_;
            float lateral = (unit(random) * 2.0f - 1.0f) * clusterSpread_;
            Vector3 position = {
                radial.x * (wallOuterRadius + distance) + tangent.x * lateral,
                0.0f,
                radial.z * (wallOuterRadius + distance) + tangent.z * lateral };

            // 位置から見た外向きの方向（傾ける向き）と、壁の外側からの実際の距離
            float fromCenter = std::sqrt(position.x * position.x + position.z * position.z);
            float outDistance = fromCenter - wallOuterRadius;
            Vector3 outward = { position.x / std::fmax(fromCenter, kMinDirectionLength), 0.0f, position.z / std::fmax(fromCenter, kMinDirectionLength) };

            // 高さは、壁の外側からの距離に比例した上限で抑える（自機が隠れないようにする）
            float height = heightMin + unit(random) * (heightMax - heightMin);
            height = std::fmin(height, std::fmax(outDistance, 0.0f) * occlusionRatio_);

            float yaw = unit(random) * kTwoPi;
            float tilt = unit(random) * maxTilt;

            // 拡縮、向き（Y回転）、外向きの傾き、移動の順に行列で合成する。
            // 傾ける軸は、上方向(Y)から外向きの方向への外積（回転が正のとき上端が外向きへ倒れる）
            Vector3 tiltAxis = { outward.z, 0.0f, -outward.x };
            Matrix4x4 world = Matrix4x4::MakeScaleMatrix({ height * thicknessScale_, height, height * thicknessScale_ })
                * Matrix4x4::MakeRotateYMatrix(yaw)
                * Matrix4x4::MakeRotateAxisAngle(tiltAxis, tilt)
                * Matrix4x4::MakeTranslateMatrix(position);

            crystals_[kind][usedCounts_[kind]]->worldTransform_.UpdateWorldMatrix(world);
            usedCounts_[kind]++;
        }
    }
}

void FieldScenery::UpdateSnowField() {
    float radius = std::fmax(field_->GetRadius(), kMinRadius);
    float scale = radius * std::fmax(snowScale_, kMinSnowScale);

    // 円盤の床の少し下に置く
    snowField_.worldTransform_.transform_.scale = { scale, 1.0f, scale };
    snowField_.worldTransform_.transform_.translate = { 0.0f, snowOffsetY_, 0.0f };

    snowField_.materialData_->color = snowColor_;
    snowField_.materialData_->roughness = snowRoughness_;
    snowField_.materialData_->metallic = snowMetallic_;

    // 画像の繰り返し回数は、一辺の長さ（planeXZは一辺2）÷1枚が覆う長さ。広さを変えても模様の細かさは変わらない
    float tileCount = scale * 2.0f / std::fmax(snowTileSize_, kMinSnowTileSize);
    snowField_.materialData_->uvTransform = Matrix4x4::MakeScaleMatrix({ tileCount, tileCount, 1.0f });
}
