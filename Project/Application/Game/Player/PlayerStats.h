#pragma once

/// <summary>
/// プレイヤーの強化対象になる値をまとめる構造体。GameSceneが所有する。
/// </summary>
struct PlayerStats {
    float moveSpeed = 8.0f; // 移動速度（ワールド単位/秒）
};
