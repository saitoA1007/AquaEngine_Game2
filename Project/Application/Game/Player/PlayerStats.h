#pragma once

/// <summary>
/// プレイヤーの強化対象になる値をまとめる構造体。GameSceneが所有する。
/// </summary>
struct PlayerStats {
    float moveSpeed = 8.0f;  // 移動速度（ワールド単位/秒）
    float rangeScale = 1.0f; // 氷エリアの通常半径とフルチャージ半径に掛ける倍率
    float powerScale = 1.0f; // 単押しとチャージの威力に掛ける倍率
};
