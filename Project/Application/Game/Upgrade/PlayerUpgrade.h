#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include "DebugParameter.h"
#include "Application/Game/Player/PlayerStats.h"

/// <summary>
/// 強化の項目。毎回すべてを並べ、1つ選ぶ。
/// </summary>
enum class UpgradeType {
    kRange,      // 氷霧と氷エリアの範囲
    kPower,      // 氷霧の威力
    kMoveSpeed,  // 移動速度
};

/// <summary>
/// 強化の項目の名前と説明、1回あたりの上昇量、何回取ったかを持ち、PlayerStatsへ反映する。
/// 上昇量はDebugParameterで調整する。Playerの保存済みの調整値は書き換えない。
/// </summary>
class PlayerUpgrade {
public:
    // 強化の項目数
    static constexpr int32_t kTypeCount = 3;

    /// <summary>
    /// 調整値を登録する。取った回数は全て0から始まる。
    /// </summary>
    PlayerUpgrade();

    /// <summary>
    /// 項目の名前を取得する。
    /// </summary>
    /// <param name="type">強化の項目</param>
    static const char* GetName(UpgradeType type);

    /// <summary>
    /// 項目の短い説明を、1回あたりの上昇量を含めて作る。
    /// </summary>
    /// <param name="type">強化の項目</param>
    std::string GetDescription(UpgradeType type) const;

    /// <summary>
    /// 項目を1回取ったことにする。
    /// </summary>
    /// <param name="type">選んだ強化の項目</param>
    void Select(UpgradeType type);

    /// <summary>
    /// 項目を何回取ったかを取得する。
    /// </summary>
    /// <param name="type">強化の項目</param>
    int32_t GetCount(UpgradeType type) const { return counts_[static_cast<size_t>(type)]; }

    /// <summary>
    /// 取った回数に応じた値をPlayerStatsへ書く。毎フレーム呼んでよい（回数から毎回求め直す）。
    /// </summary>
    /// <param name="stats">書き込み先</param>
    /// <param name="baseMoveSpeed">強化前の移動速度（調整値）。ここへ上昇分を足す</param>
    void ApplyToStats(PlayerStats& stats, float baseMoveSpeed) const;

    /// <summary>
    /// 調整値の反映を行う。毎フレーム呼ぶ。
    /// </summary>
    void UpdateParameters();

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

private:
    // 調整値
    float rangeStep_ = 0.15f;     // 範囲を1回取るごとに増える倍率（0.15で+15%）（仮）
    float powerStep_ = 0.2f;      // 威力を1回取るごとに増える倍率（0.2で+20%）（仮）
    float moveSpeedStep_ = 1.0f;  // 移動速度を1回取るごとに足す量（ワールド単位/秒）（仮）

    // 状態
    std::array<int32_t, kTypeCount> counts_ = {}; // 項目ごとに取った回数

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};
