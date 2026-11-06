#pragma once
#include "Application/Game/Enemy/Enemy.h"

/// <summary>
/// プレイヤーへただ迫る敵。迫る敵と速い敵は、敵一覧の移動速度などの違いだけで区別する。
/// </summary>
class ChaseEnemy : public Enemy {
protected:
    /// <summary>
    /// プレイヤーの方向へ直進する。
    /// </summary>
    void UpdateBehavior(const EnemyContext& context) override;
};
