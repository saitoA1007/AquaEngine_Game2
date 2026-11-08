#pragma once
#include <cstdint>
#include "Vector3.h"
#include "Application/Game/Enemy/Enemy.h"

/// <summary>
/// 敵が毎フレーム読むプレイヤーの情報。GameSceneが作り、EnemyManagerは保持しない。
/// </summary>
struct EnemyInput {
    Vector3 playerPosition = { 0.0f, 0.0f, 0.0f }; // プレイヤーのワールド位置
    float playerRadius = 0.5f;                     // プレイヤーの半径
    float iceAreaRadius = 0.0f;                    // 氷エリアの半径（プレイヤー中心）
    float fieldRadius = 0.0f;                      // フィールドの半径（中心は原点）
};

/// <summary>
/// 敵の更新結果。GameSceneが受け取り、Playerへ反映する。
/// </summary>
struct EnemyOutput {
    int32_t damage = 0;                      // プレイヤーが受けるダメージ（0なら被弾なし）
    Vector3 pushback = { 0.0f, 0.0f, 0.0f }; // プレイヤーを押し戻す量（凍った敵との重なり分）
};

/// <summary>
/// 敵への参照。プールの個体は再利用されるため、ポインタと出現ごとの連番を組にして、同じ敵かを確かめられるようにする。
/// 有効かどうかはEnemyManagerが判定する。
/// </summary>
struct EnemyTarget {
    Enemy* enemy = nullptr; // 敵の個体（EnemyManagerが所有する）
    uint32_t serial = 0;    // 取得時の出現ごとの連番
};

/// <summary>
/// 敵を撃破したときの通知内容。EnemyManagerが作り、コールバックで渡す。
/// </summary>
struct EnemyDefeatInfo {
    Vector3 position = { 0.0f, 0.0f, 0.0f }; // 撃破した位置（XZを使う）
    int32_t iceShardCount = 0;               // 落とす氷塊の数
    int32_t exp = 0;                         // 与える経験値（敵一覧のexp）
    uint32_t chainId = 0;                    // 連鎖数の数え上げに使うID（氷塊で倒したときは発射元の氷霧のID。氷霧で倒したときは0で、数えない）
};
