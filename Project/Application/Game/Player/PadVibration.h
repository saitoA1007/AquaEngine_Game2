#pragma once
#include <memory>
#include "InputCommand.h"
#include "DebugParameter.h"

/// <summary>
/// コントローラーの振動。チャージ中の連続した振動と、氷霧・命中の短い振動をまとめ、重なったときは最大値を出す。
/// 左右のモーターは同じ値で、強さと長さはDebugParameterで調整する。
/// エンジンの振動は次に呼ぶまで続くため、毎フレームのUpdateで長さを管理し、破棄時に止める。
/// </summary>
class PadVibration {
public:
    /// <summary>
    /// 調整値を登録する。
    /// </summary>
    /// <param name="inputCommand">振動の出力先。シーンが所有し、このオブジェクトより長く生存する。</param>
    explicit PadVibration(GameEngine::InputCommand* inputCommand);

    /// <summary>
    /// 振動を止める。
    /// </summary>
    ~PadVibration();

    /// <summary>
    /// 長押しでチャージ中かを設定する。trueの間、チャージの強さで振動し続ける。毎フレーム呼ぶ。
    /// </summary>
    /// <param name="isCharging">チャージ中か</param>
    void SetCharging(bool isCharging) { isCharging_ = isCharging; }

    /// <summary>
    /// 氷霧を発生させたときの振動を始める。
    /// </summary>
    void PlayRelease() { releaseTimer_ = releaseTime_; }

    /// <summary>
    /// 氷塊が敵に命中したときの振動を始める。命中のたびに時間が延びる。
    /// </summary>
    void PlayHit() { hitTimer_ = hitTime_; }

    /// <summary>
    /// 振動の長さを進め、そのフレームの出力を最大値で決めてコントローラーへ送る。値が変わったときだけ送る。
    /// </summary>
    void Update();

    /// <summary>
    /// 全ての振動を止める。
    /// </summary>
    void Stop();

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// コントローラーへ振動の強さを送る。前回と同じ値なら送らない。
    /// </summary>
    /// <param name="strength">左右共通の強さ（0から1）</param>
    void Apply(float strength);

private:
    // 調整値
    bool isEnabled_ = true;           // 振動を出すか
    float chargeStrength_ = 0.2f;     // 長押しでチャージ中の強さ
    float releaseStrength_ = 0.6f;    // 離して氷霧を発生させたときの強さ
    float hitStrength_ = 0.35f;       // 氷塊が敵に命中したときの強さ
    float releaseTime_ = 0.15f;       // 氷霧の振動の長さ（秒）
    float hitTime_ = 0.08f;           // 命中の振動の長さ（秒）

    // 状態
    bool isCharging_ = false;         // 長押しでチャージ中か
    float releaseTimer_ = 0.0f;       // 氷霧の振動の残り時間（秒）
    float hitTimer_ = 0.0f;           // 命中の振動の残り時間（秒）
    float lastStrength_ = 0.0f;       // 最後に送った強さ

    // 参照（所有しない）
    GameEngine::InputCommand* inputCommand_ = nullptr;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};
