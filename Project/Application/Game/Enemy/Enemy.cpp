#include "pch.h"
#include "Enemy.h"
#include <algorithm>

void Enemy::Spawn(const EnemyDefinition* definition, const Vector3& position, uint32_t serial) {
    definition_ = definition;
    serial_ = serial;
    position_ = { position.x, 0.0f, position.z };
    moveDirection_ = { 0.0f, 0.0f, 0.0f };

    // 状態を初期化して使い始める
    isActive_ = true;
    state_ = EnemyState::kNormal;
    iceGauge_ = 0.0f;
    frozenTimer_ = 0.0f;
    wallSense_ = 0;
    lastMistId_ = 0;
    brokenCount_ = 0;
    justReleased_ = false;

    OnSpawn();
}

void Enemy::Update(const EnemyContext& context) {
    // 凍っていれば、解放までの時間だけ進める
    if (state_ == EnemyState::kFrozen) {
        frozenTimer_ -= context.deltaTime;
        if (frozenTimer_ <= 0.0f) {
            Release();
        }
        return;
    }

    UpdateIceGauge(context);

    // 凍った直後のフレームは動かない
    if (state_ == EnemyState::kFrozen) {
        return;
    }

    UpdateBehavior(context);
}

void Enemy::UpdateIceGauge(const EnemyContext& context) {
    // 氷エリアの内外を、プレイヤーとの距離で判定する
    float dx = position_.x - context.playerPosition.x;
    float dz = position_.z - context.playerPosition.z;
    bool isInIceArea = (dx * dx + dz * dz) <= context.iceAreaRadius * context.iceAreaRadius;

    if (isInIceArea) {
        iceGauge_ += context.freezeRate * context.deltaTime;
    } else {
        iceGauge_ = (std::max)(iceGauge_ - context.decayRate * context.deltaTime, 0.0f);
    }

    // 閾値に達したら凍る
    if (iceGauge_ >= definition_->iceThreshold) {
        Freeze(context.frozenDuration);
    }
}

void Enemy::ApplyIceDamage(float amount, uint32_t mistId, float frozenDuration) {
    // 凍っている敵への氷ダメージは、砕く処理が扱う
    if (state_ == EnemyState::kFrozen) {
        return;
    }

    iceGauge_ += amount;
    if (iceGauge_ < definition_->iceThreshold) {
        return;
    }

    // この氷霧で凍ったことを覚え、同じ氷霧では砕かれないようにする
    Freeze(frozenDuration);
    lastMistId_ = mistId;
}

bool Enemy::Shatter() {
    brokenCount_++;

    // 必要な回数に達したら撃破。個体を使い終える
    if (brokenCount_ >= definition_->breakCount) {
        isActive_ = false;
        return true;
    }

    // 未満なら氷だけ砕けて解放される
    // 氷霧で砕かれた解放だけ、殻の破片のイベントを残す（自然解放は溶ける表現で、破片は出さない）
    Release();
    justReleased_ = true;
    return false;
}

void Enemy::Freeze(float frozenDuration) {
    state_ = EnemyState::kFrozen;
    iceGauge_ = definition_->iceThreshold;
    frozenTimer_ = frozenDuration;
    lastMistId_ = 0;
    moveDirection_ = { 0.0f, 0.0f, 0.0f };
    OnFrozen();
}

void Enemy::Release() {
    state_ = EnemyState::kNormal;
    iceGauge_ = 0.0f;
    frozenTimer_ = 0.0f;
    lastMistId_ = 0;
    OnReleased();
}
