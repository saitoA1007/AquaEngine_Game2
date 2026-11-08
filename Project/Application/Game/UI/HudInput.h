#pragma once
#include <cstdint>

/// <summary>
/// GameHudが毎フレーム受け取る表示用の値。GameSceneが作る。GameHudはゲームのクラスを知らない。
/// </summary>
struct HudInput {
    int32_t hp = 0;          // 現在のHP
    int32_t maxHp = 0;       // 最大HP（HPアイコンの数）
    int32_t phase = 1;       // 現在のフェーズ
};
