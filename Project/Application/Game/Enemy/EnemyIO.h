#pragma once
#include <cstdint>
#include "Vector3.h"

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
