#include "pch.h"
#include "ChaseEnemy.h"
#include <cmath>

void ChaseEnemy::UpdateBehavior(const EnemyContext& context) {
    float dx = context.playerPosition.x - position_.x;
    float dz = context.playerPosition.z - position_.z;
    float distance = std::sqrt(dx * dx + dz * dz);

    // 重なっていて向きが決まらないときは動かない（0除算を避ける）
    if (distance < 0.0001f) {
        moveDirection_ = { 0.0f, 0.0f, 0.0f };
        return;
    }

    moveDirection_ = { dx / distance, 0.0f, dz / distance };
    float step = definition_->moveSpeed * context.deltaTime;
    position_.x += moveDirection_.x * step;
    position_.z += moveDirection_.z * step;
}
