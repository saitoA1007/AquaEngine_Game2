#include "pch.h"
#include "IceAreaView.h"
#include "MyMath.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
using namespace GameEngine;

IceAreaView::IceAreaView(const IceAreaAssets& assets) : assets_(assets) {
    if (assets_.slab == nullptr || assets_.plane == nullptr) {
        throw std::runtime_error("IceAreaView: model is missing");
    }

    slabTransform_.Initialize({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    CreateSlabBuffers();
    CreateInstances();
    RegisterDebugParameters();
}

void IceAreaView::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>(kDebugName);

    // サブグループ名。ここを書き換えれば登録の全てに反映される
    const std::string kSlabGroup = "Slab";
    const std::string kEdgeGroup = "Edge";
    const std::string kFlakeGroup = "Flake";
    const std::string kBurstGroup = "Burst";

    debugParam_->Register("slabThickness", slabThickness_, 0, kSlabGroup);
    debugParam_->Register("slabColor", slabColor_, 1, kSlabGroup);
    debugParam_->Register("highlightStrength", highlightStrength_, 2, kSlabGroup);
    debugParam_->Register("highlightSize", highlightSize_, 3, kSlabGroup);
    debugParam_->Register("highlightOffset", highlightOffset_, 4, kSlabGroup);

    debugParam_->Register("edgeWidth", edgeWidth_, 0, kEdgeGroup);
    debugParam_->Register("edgeSegmentLength", edgeSegmentLength_, 1, kEdgeGroup);
    debugParam_->Register("edgeColor", edgeColor_, 2, kEdgeGroup);
    debugParam_->Register("edgeBrightness", edgeBrightness_, 3, kEdgeGroup);
    debugParam_->Register("edgeLineWidth", edgeLineWidth_, 4, kEdgeGroup);
    debugParam_->Register("edgeLineColor", edgeLineColor_, 5, kEdgeGroup);
    debugParam_->Register("layerLift", layerLift_, 6, kEdgeGroup);

    debugParam_->Register("flakeSize", flakeSize_, 0, kFlakeGroup);
    debugParam_->Register("flakeSmallRatio", flakeSmallRatio_, 1, kFlakeGroup);
    debugParam_->Register("flakeSpacing", flakeSpacing_, 2, kFlakeGroup);
    debugParam_->Register("flakeInset", flakeInset_, 3, kFlakeGroup);
    debugParam_->Register("flakeSpinSpeed", flakeSpinSpeed_, 4, kFlakeGroup);
    debugParam_->Register("flakeColor", flakeColor_, 5, kFlakeGroup);
    debugParam_->Register("flakeBrightness", flakeBrightness_, 6, kFlakeGroup);

    debugParam_->Register("burstTime", burstTime_, 0, kBurstGroup);
    debugParam_->Register("burstSpread", burstSpread_, 1, kBurstGroup);
    debugParam_->Register("burstBrightness", burstBrightness_, 2, kBurstGroup);
    debugParam_->Register("edgeFlashBoost", edgeFlashBoost_, 3, kBurstGroup);
    debugParam_->Apply();
}

void IceAreaView::CreateSlabBuffers() {
    // 光を使わない白い材質。絵はテクスチャで描き、色は調整値で掛ける
    slabMaterial_.Initialize({ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f, false);
    slabMaterial_.GetMaterialData()->textureHandle = assets_.slabTexture;

    // 氷用のヒットグループとマスクは使わず、通常のレイトレーシングの描画にする
    slabRefBuffers_.resize(assets_.slab->GetMeshes().size());
    for (RefBuffer& refBuffer : slabRefBuffers_) {
        refBuffer.Create();
        refBuffer.SetBufferMaterial(static_cast<uint32_t>(BufferType::kDefalutMaterial), slabMaterial_.GetMaterialSrvIndex());
    }
}

void IceAreaView::CreateInstances() {
    Transform identity = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    highlightTransforms_.Initialize(1, identity);
    edgeTransforms_.Initialize(kMaxEdgeSegments * 2, identity);
    flakeTransforms_.Initialize(kMaxFlakes, identity);
    burstTransforms_.Initialize(kMaxEdgeSegments, identity);

    // テクスチャはインスタンスごとに決める。ここで1度だけ入れておく
    highlightTransforms_.transformDatas_[0].textureHandle = assets_.slabHighlight;
    for (WorldTransforms::TransformData& data : edgeTransforms_.transformDatas_) {
        data.textureHandle = assets_.rimLine;
    }
    for (WorldTransforms::TransformData& data : burstTransforms_.transformDatas_) {
        data.textureHandle = assets_.rimLine;
    }
    for (WorldTransforms::TransformData& data : flakeTransforms_.transformDatas_) {
        data.textureHandle = assets_.snowflake;
    }
}

void IceAreaView::Update(float deltaTime) {
    debugParam_->ApplyIfDirty();

    // 結晶の並び全体をゆっくり回す
    spinAngle_ += flakeSpinSpeed_ * deltaTime;

    // 発動時の光の時間を進める
    burstTimer_ = (std::max)(burstTimer_ - deltaTime, 0.0f);
}

void IceAreaView::PlayBurst(const Vector3& center, float radius) {
    burstCenter_ = center;
    burstStartRadius_ = radius;
    burstTimer_ = burstTime_;
}

void IceAreaView::Draw(RenderQueue* renderQueue, const Vector3& center, float radius) {
    DrawSlab(renderQueue, center, radius);
    DrawHighlight(renderQueue, center, radius);
    DrawEdge(renderQueue, center, radius);
    DrawFlakes(renderQueue, center, radius);
    DrawBurst(renderQueue);
}

void IceAreaView::DrawSlab(RenderQueue* renderQueue, const Vector3& center, float radius) {
    slabMaterial_.GetMaterialData()->color = slabColor_;

    // 板は底が地面。XZを半径に合わせて広げ、厚みは一定にする
    float safeRadius = (std::max)(radius, 0.01f);
    Vector3 scale = { safeRadius, (std::max)(slabThickness_, 0.001f), safeRadius };
    slabTransform_.SetWorldMatrix(Math::MakeWorldMatrixFromEulerRotation({ center.x, 0.0f, center.z }, { 0.0f, 0.0f, 0.0f }, scale));
    renderQueue->SubmitRaytracingModel(assets_.slab, slabTransform_, &slabRefBuffers_);
}

Matrix4x4 IceAreaView::MakePlateMatrix(const Vector3& position, const Vector3& tangent, float halfLength, float halfWidth) {
    // 行は、モデルのX軸・Y軸・Z軸が移る先。板のZ軸は、Y軸まわりにX軸を90度回した向き
    Matrix4x4 matrix = Matrix4x4::MakeIdentity();
    matrix.m[0][0] = tangent.x * halfLength;
    matrix.m[0][1] = 0.0f;
    matrix.m[0][2] = tangent.z * halfLength;
    matrix.m[2][0] = -tangent.z * halfWidth;
    matrix.m[2][1] = 0.0f;
    matrix.m[2][2] = tangent.x * halfWidth;
    matrix.m[3][0] = position.x;
    matrix.m[3][1] = position.y;
    matrix.m[3][2] = position.z;
    return matrix;
}

void IceAreaView::DrawHighlight(RenderQueue* renderQueue, const Vector3& center, float radius) {
    if (highlightStrength_ <= 0.0f) {
        return;
    }

    // 位置と大きさは半径に比例させ、板の絵と同じように引き伸ばす
    Vector3 position = {
        center.x + highlightOffset_.x * radius,
        slabThickness_ + layerLift_ * 0.5f,
        center.z + highlightOffset_.z * radius };
    Vector3 tangent = { std::cos(kHighlightAngle), 0.0f, std::sin(kHighlightAngle) };
    float halfLength = (std::max)(highlightSize_ * radius, 0.01f);

    WorldTransforms::TransformData& data = highlightTransforms_.transformDatas_[0];
    data.worldMatrix = MakePlateMatrix(position, tangent, halfLength, halfLength * kHighlightAspect);
    data.color = { 1.0f, 1.0f, 1.0f, highlightStrength_ };
    renderQueue->SubmitInstancing(assets_.plane, 1, highlightTransforms_, 0.0f, BlendMode::kBlendModeAdd, nullptr, kAddPassName);
}

uint32_t IceAreaView::FillEdgeBand(WorldTransforms& transforms, uint32_t startIndex, const Vector3& center, float ringRadius, float width, float y, const Vector4& color) const {
    const float kTwoPi = 6.28318530718f;
    float safeRadius = (std::max)(ringRadius, 0.01f);

    // 円周を帯の長さ付近で等分する。数が増えても帯の長さと太さはほぼ一定
    uint32_t count = static_cast<uint32_t>(std::ceil(kTwoPi * safeRadius / (std::max)(edgeSegmentLength_, 0.1f)));
    count = std::clamp(count, kMinCount, kMaxEdgeSegments);

    float step = kTwoPi / static_cast<float>(count);
    float halfLength = safeRadius * step * 0.5f * kEdgeOverlap;
    float halfWidth = (std::max)(width, 0.005f) * 0.5f;
    for (uint32_t i = 0; i < count; ++i) {
        float angle = step * static_cast<float>(i);
        Vector3 radial = { std::cos(angle), 0.0f, std::sin(angle) };
        Vector3 tangent = { -radial.z, 0.0f, radial.x };
        Vector3 position = { center.x + radial.x * safeRadius, y, center.z + radial.z * safeRadius };

        WorldTransforms::TransformData& data = transforms.transformDatas_[startIndex + i];
        data.worldMatrix = MakePlateMatrix(position, tangent, halfLength, halfWidth);
        data.color = color;
    }
    return count;
}

void IceAreaView::DrawEdge(RenderQueue* renderQueue, const Vector3& center, float radius) {
    // 発動の直後は、縁を明るくして、時間とともに元へ戻す
    float flash = 0.0f;
    if (burstTimer_ > 0.0f) {
        flash = edgeFlashBoost_ * (burstTimer_ / (std::max)(burstTime_, 0.01f));
    }
    float brightness = edgeBrightness_ * (1.0f + flash);
    Vector4 color = { edgeColor_.x * brightness, edgeColor_.y * brightness, edgeColor_.z * brightness, edgeColor_.w };
    float y = slabThickness_ + layerLift_;

    // 太い光の帯は、外側が板の端に合うよう、太さの半分だけ内側に通す
    uint32_t bandCount = FillEdgeBand(edgeTransforms_, 0, center, radius - edgeWidth_ * 0.5f, edgeWidth_, y, color);

    // 細い白い線は、太い帯の内側の境目に通す。同じバッファの続きに書き、1回で描く
    Vector4 lineColor = { edgeLineColor_.x, edgeLineColor_.y, edgeLineColor_.z, edgeLineColor_.w };
    uint32_t lineCount = FillEdgeBand(edgeTransforms_, bandCount, center, radius - edgeWidth_, edgeLineWidth_, y + kFlakeLift * 0.5f, lineColor);
    renderQueue->SubmitInstancing(assets_.plane, bandCount + lineCount, edgeTransforms_, 0.0f, BlendMode::kBlendModeAdd, nullptr, kAddPassName);
}

void IceAreaView::DrawFlakes(RenderQueue* renderQueue, const Vector3& center, float radius) {
    const float kTwoPi = 6.28318530718f;
    float ringRadius = (std::max)(radius - flakeInset_, 0.01f);

    // 円周を間隔付近で等分する。大小を交互に並べるため、数は偶数にそろえる
    uint32_t count = static_cast<uint32_t>(std::floor(kTwoPi * ringRadius / (std::max)(flakeSpacing_, 0.1f)));
    count = std::clamp(count & ~1u, kMinCount, kMaxFlakes);

    float step = kTwoPi / static_cast<float>(count);
    float y = slabThickness_ + layerLift_ + kFlakeLift;
    Vector4 color = {
        flakeColor_.x * flakeBrightness_, flakeColor_.y * flakeBrightness_, flakeColor_.z * flakeBrightness_, flakeColor_.w };
    for (uint32_t i = 0; i < count; ++i) {
        // 並び全体を回す。結晶の向きは、円の接線に合わせて一緒に回る
        float angle = step * static_cast<float>(i) + spinAngle_;
        Vector3 radial = { std::cos(angle), 0.0f, std::sin(angle) };
        Vector3 tangent = { -radial.z, 0.0f, radial.x };
        Vector3 position = { center.x + radial.x * ringRadius, y, center.z + radial.z * ringRadius };

        // 偶数番目は大きく、奇数番目は小さくする
        float size = (i % 2 == 0) ? flakeSize_ : flakeSize_ * flakeSmallRatio_;
        float half = (std::max)(size, 0.01f) * 0.5f;

        WorldTransforms::TransformData& data = flakeTransforms_.transformDatas_[i];
        data.worldMatrix = MakePlateMatrix(position, tangent, half, half);
        data.color = color;
    }
    renderQueue->SubmitInstancing(assets_.plane, count, flakeTransforms_, 0.0f, BlendMode::kBlendModeAdd, nullptr, kAddPassName);
}

void IceAreaView::DrawBurst(RenderQueue* renderQueue) {
    if (burstTimer_ <= 0.0f) {
        return;
    }

    // 経過の割合。広がりは最初が速く、明るさは直線で薄くなる
    float progress = 1.0f - burstTimer_ / (std::max)(burstTime_, 0.01f);
    float eased = 1.0f - (1.0f - progress) * (1.0f - progress);
    float fade = 1.0f - progress;
    float radius = burstStartRadius_ + burstSpread_ * eased;

    float brightness = burstBrightness_ * fade;
    Vector4 color = { edgeColor_.x * brightness, edgeColor_.y * brightness, edgeColor_.z * brightness, edgeColor_.w * fade };
    float y = slabThickness_ + layerLift_ + kBurstLift;
    uint32_t count = FillEdgeBand(burstTransforms_, 0, burstCenter_, radius - edgeWidth_ * 0.5f, edgeWidth_, y, color);
    renderQueue->SubmitInstancing(assets_.plane, count, burstTransforms_, 0.0f, BlendMode::kBlendModeAdd, nullptr, kAddPassName);
}
