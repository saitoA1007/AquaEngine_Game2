#pragma once
#include <cstdint>
#include "Vector3.h"

/// <summary>
/// 氷霧1発分の情報。Playerが作り、コールバックでGameSceneへ渡す。Playerは敵の中身を知らない。
/// </summary>
struct IceMistInfo {
    uint32_t id = 0;                               // 発生ごとの連番（1から。0は「なし」を表す）
    Vector3 center = { 0.0f, 0.0f, 0.0f };         // 氷霧の中心（XZを使う）
    float radius = 0.0f;                           // 氷霧の半径
    float power = 0.0f;                            // 威力（敵の氷ゲージへ足す量）
};
