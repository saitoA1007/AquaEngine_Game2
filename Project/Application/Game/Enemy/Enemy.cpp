#include "pch.h"
#include "Enemy.h"
#include <algorithm>

void Enemy::Spawn(const EnemyDefinition* definition, const Vector3& position) {
    definition_ = definition;
    position_ = { position.x, 0.0f, position.z };
    moveDirection_ = { 0.0f, 0.0f, 0.0f };

    // 状態を初期化して使い始める
    isActive_ = true;
    state_ = EnemyState::kNormal;
    iceGauge_ = 0.0f;
    frozenTimer_ = 0.0f;
    wallSense_ = 0;

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
        Freeze(context);
    }
}

void Enemy::Freeze(const EnemyContext& context) {
    state_ = EnemyState::kFrozen;
    iceGauge_ = definition_->iceThreshold;
    frozenTimer_ = context.frozenDuration;
    moveDirection_ = { 0.0f, 0.0f, 0.0f };
    OnFrozen();
}

void Enemy::Release() {
    state_ = EnemyState::kNormal;
    iceGauge_ = 0.0f;
    frozenTimer_ = 0.0f;
    OnReleased();
}
