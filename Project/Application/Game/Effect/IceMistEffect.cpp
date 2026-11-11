#include "pch.h"
#include "IceMistEffect.h"
#include "FPSCounter.h"
#include "RandomGenerator.h"
#include <algorithm>
#include <cmath>
#include <numbers>
using namespace GameEngine;

IceMistEffect::IceMistEffect(Model* planeModel, TextureManager* textureManager, uint32_t whiteTextureHandle) {
    RegisterDebugParameters();

    // 氷霧のたびにEmitで発生させるため、どちらもループは無効にする
    burst_ = std::make_unique<ParticleBehavior>(kBurstParticleName, kMaxBurstParticles, textureManager, planeModel);
    burst_->Initialize();
    burst_->SetIsLoop(false);

    rise_ = std::make_unique<ParticleBehavior>(kRiseParticleName, kMaxRiseParticles, textureManager, planeModel);
    rise_->Initialize();
    rise_->SetIsLoop(false);

    // 画面の光は全画面の板に色を掛けて作る。最初は透明
    flashSprite_ = std::make_unique<Sprite>(Vector2{ 0.0f, 0.0f }, Vector2{ kScreenWidth, kScreenHeight }, Vector2{ 0.0f, 0.0f }, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });
    flashSprite_->textureHandle_ = whiteTextureHandle;
}

void IceMistEffect::RegisterDebugParameters() {
    // サブグループ名。ここを書き換えれば登録の全てに反映される
    const std::string kBurstGroup = "Burst";
    const std::string kRiseGroup = "Rise";
    const std::string kFlashGroup = "Flash";

    debugParam_ = std::make_unique<DebugParameter>(kDebugName);

    // 霧の吹き出し
    debugParam_->Register("startAreaRatio", startAreaRatio_, 0, kBurstGroup);
    debugParam_->Register("minTravelRatio", minTravelRatio_, 1, kBurstGroup);
    debugParam_->Register("tapRadius", tapRadius_, 3, kBurstGroup);
    debugParam_->Register("maxRadius", maxRadius_, 4, kBurstGroup);
    debugParam_->Register("tapEndSize", tapEndSize_, 5, kBurstGroup);
    debugParam_->Register("maxEndSize", maxEndSize_, 6, kBurstGroup);
    debugParam_->Register("tapCount", tapCount_, 7, kBurstGroup);
    debugParam_->Register("maxCount", maxCount_, 8, kBurstGroup);
    debugParam_->Register("lifeVariation", lifeVariation_, 9, kBurstGroup);
    debugParam_->Register("burstHeight", burstHeight_, 10, kBurstGroup);

    // 冷気
    debugParam_->Register("riseCount", riseCount_, 0, kRiseGroup);
    debugParam_->Register("riseHeight", riseHeight_, 1, kRiseGroup);

    // 画面の光
    debugParam_->Register("flashColor", flashColor_, 0, kFlashGroup);
    debugParam_->Register("flashMaxAlpha", flashMaxAlpha_, 1, kFlashGroup);
    debugParam_->Register("flashTime", flashTime_, 2, kFlashGroup);

    debugParam_->Apply();
}

void IceMistEffect::Update() {
    debugParam_->ApplyIfDirty();

    burst_->Update();
    rise_->Update();
    UpdateFlash();
}

void IceMistEffect::Draw() {
    burst_->Draw();
    rise_->Draw();

    // 光っていない間は板を出さない
    if (isFlashing_) {
        renderQueue_->SubmitSprite(flashSprite_.get());
    }
}

void IceMistEffect::Play(const Vector3& center, float radius) {
    float chargeRatio = CalcChargeRatio(radius);
    BurstShape shape = CalcBurstShape(radius, chargeRatio);

    EmitBurst(center, shape);
    EmitRise(center, radius);
    StartFlash();
}

float IceMistEffect::CalcChargeRatio(float radius) const {
    // 半径の範囲が潰れているときは0除算を避けて、チャージなしとして扱う
    float range = maxRadius_ - tapRadius_;
    if (range <= 0.0001f) {
        return 0.0f;
    }
    return std::clamp((radius - tapRadius_) / range, 0.0f, 1.0f);
}

IceMistEffect::BurstShape IceMistEffect::CalcBurstShape(float radius, float chargeRatio) const {
    BurstShape shape;
    shape.radius = radius;
    shape.chargeRatio = chargeRatio;
    shape.countPerDirection = (std::max)(static_cast<int32_t>(std::lround(tapCount_ + (maxCount_ - tapCount_) * chargeRatio)), 1);
    shape.endSize = tapEndSize_ + (maxEndSize_ - tapEndSize_) * chargeRatio;
    return shape;
}

void IceMistEffect::EmitBurst(const Vector3& center, const BurstShape& shape) {
    // 粒の最終の大きさは、今回の吹き出しに合わせる
    burst_->SetSizeOverLifeTime(shape.endSize);

    // 毎回同じ向きにならないよう、全体を少し回してから等間隔に並べる
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    float phase = RandomGenerator::Get(0.0f, kTwoPi / static_cast<float>(kDirectionCount));
    for (int32_t i = 0; i < kDirectionCount; ++i) {
        float angle = phase + kTwoPi * static_cast<float>(i) / static_cast<float>(kDirectionCount);
        float cosAngle = std::cos(angle);
        float sinAngle = std::sin(angle);
        burst_->SetDirection({ cosAngle, 0.0f, sinAngle });

        for (int32_t k = 0; k < shape.countPerDirection; ++k) {
            EmitBurstParticle(center, shape, cosAngle, sinAngle);
        }
    }
}

void IceMistEffect::EmitBurstParticle(const Vector3& center, const BurstShape& shape, float cosAngle, float sinAngle) {
    // 出し始める位置は、中心から範囲の内側に面積が均等になるよう散らす（中心にも霧が出る）
    float startDistance = shape.radius * startAreaRatio_ * std::sqrt(RandomGenerator::Get(0.0f, 1.0f));

    // 縁で止まるよう、流れる距離を「縁までの残り」にする（縁の近くから出た粒も少しは動く）
    float travelDistance = (std::max)(shape.radius - startDistance, shape.radius * minTravelRatio_);

    // 初速は固定なので、流れる距離から止まるまでの時間が決まり、粒の寿命もそこから決まる
    float arriveTime = travelDistance / (kBurstSpeed * kStopDistanceRatio);
    float lifeTime = arriveTime / kMoveEndRatio;
    burst_->SetLifeTime(lifeTime * (1.0f - lifeVariation_), lifeTime * (1.0f + lifeVariation_));

    Vector3 emitPosition = {
        center.x + cosAngle * startDistance,
        burstHeight_,
        center.z + sinAngle * startDistance };
    burst_->SetEmitterPos(emitPosition);
    burst_->Emit(emitPosition);
}
void IceMistEffect::EmitRise(const Vector3& center, float radius) {
    // 氷エリアの円の範囲から発生させる
    Vector3 emitPosition = { center.x, riseHeight_, center.z };
    rise_->SetShapeEmit(radius);
    rise_->SetEmitterPos(emitPosition);
    for (int32_t i = 0; i < riseCount_; ++i) {
        rise_->Emit(emitPosition);
    }
}

void IceMistEffect::StartFlash() {
    flashTimer_ = 0.0f;
    isFlashing_ = true;

    // 発生したフレームの描画から最大の濃さで出す（Playは更新の後に呼ばれるため、次の更新を待たない）
    flashSprite_->color_ = { flashColor_.x, flashColor_.y, flashColor_.z, flashMaxAlpha_ };
    flashSprite_->Update();
}

void IceMistEffect::UpdateFlash() {
    if (!isFlashing_) {
        return;
    }

    // ゲームの時間で進めるため、強化の選択中は止まる
    flashTimer_ += FpsCounter::GetGameDeltaTime();
    float safeTime = (std::max)(flashTime_, 0.01f);
    if (flashTimer_ >= safeTime) {
        isFlashing_ = false;
        return;
    }

    // 最大の濃さから透明へ、一定の割合で薄くする
    float remaining = 1.0f - flashTimer_ / safeTime;
    flashSprite_->color_ = { flashColor_.x, flashColor_.y, flashColor_.z, flashMaxAlpha_ * remaining };
    flashSprite_->Update();
}
