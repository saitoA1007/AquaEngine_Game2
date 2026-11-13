#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include "IGameObject.h"
#include "Application/Game/Enemy/EnemyDefinition.h"
#include "Application/Game/Phase/PhaseTable.h"
#ifdef USE_IMGUI
#include "Application/Game/Phase/PhaseTableEditor.h"
#endif

/// <summary>
/// 経験値とフェーズを管理する。経験値が必要量を超えると、その場でフェーズが進む。
/// フェーズ表を持つが、PlayerStats、EnemyManagerは知らない（GameSceneが間をつなぐ）。
/// 毎フレームの処理は持たない。
/// </summary>
class PhaseDirector : public GameEngine::IGameObject {
public:
    /// <summary>
    /// フェーズ表を読み込む。ファイルがない、または検証に失敗したときは例外を投げる。
    /// </summary>
    /// <param name="definitions">重みのIDの検証と編集画面に使う敵一覧。コピーして保持する。</param>
    explicit PhaseDirector(const std::vector<EnemyDefinition>& definitions);
    ~PhaseDirector() override = default;

    /// <summary>
    /// フェーズ表の編集画面と、現在の状態を表示する。USE_IMGUIのときだけ動く。
    /// </summary>
    void DebugUpdate() override;

    /// <summary>
    /// 経験値を足す。そのフェーズの必要量を超えるたびにフェーズを1つ進め、必要量を引く。余りは繰り越す。
    /// 1回で複数フェーズ分が入ったときは続けて進める。進んだら出現ルールの変更を1回通知し、ボスのフェーズに入ったらボス開始を通知する。
    /// ボスのフェーズ中は無視する。最終フェーズを超える分は数えない。
    /// </summary>
    /// <param name="amount">足す経験値（0以下は無視）</param>
    void AddExp(int32_t amount);

    /// <summary>
    /// 指定したフェーズへ飛ぶ（デバッグ用）。経験値は0に戻す。
    /// 出現ルールの変更とボス開始を通知する。
    /// </summary>
    /// <param name="phase">飛び先のフェーズ番号（1から表の行数まで）</param>
    void JumpToPhase(int32_t phase);

    /// <summary>
    /// 現在のフェーズを取得する（1から）。
    /// </summary>
    int32_t GetPhase() const { return phase_; }

    /// <summary>
    /// 現在のフェーズの経験値の進み具合を0から1で取得する。ボスのフェーズでは1。
    /// </summary>
    float GetExpRatio() const;

    /// <summary>
    /// 現在のフェーズがボーナスかを返す。
    /// </summary>
    bool IsBonusPhase() const { return GetCurrentRow().type == PhaseType::kBonus; }

    /// <summary>
    /// 現在のフェーズがボスかを返す。
    /// </summary>
    bool IsBossPhase() const { return GetCurrentRow().type == PhaseType::kBoss; }

    /// <summary>
    /// 現在のフェーズの表の行を取得する。
    /// </summary>
    const PhaseRow& GetCurrentRow() const { return table_.phases[static_cast<size_t>(phase_ - 1)]; }

    /// <summary>
    /// ボスのフェーズに入ったときに呼ぶ関数を設定する。ボスの中身を作る側が使う。
    /// </summary>
    /// <param name="callback">ボス開始を受け取る関数。呼び出し先がこのオブジェクトより長く生存する必要がある。</param>
    void SetBossStartCallback(const std::function<void()>& callback) { bossStartCallback_ = callback; }

    /// <summary>
    /// 現在のフェーズの出現ルールが変わったとき（編集画面の保存、デバッグの飛び先）に呼ぶ関数を設定する。
    /// 経験値でフェーズが進んだときにも呼ぶ（1回のAddExpで複数進んでも1回だけ）。
    /// </summary>
    /// <param name="callback">変更を受け取る関数。呼び出し先がこのオブジェクトより長く生存する必要がある。</param>
    void SetRuleChangedCallback(const std::function<void()>& callback) { ruleChangedCallback_ = callback; }

private:
    /// <summary>
    /// 現在のフェーズがボスなら、ボス開始を通知する。
    /// </summary>
    void NotifyBossIfNeeded();

private:
    // 状態
    PhaseTable table_;                  // 読み込んだフェーズ表（編集画面で保存されると差し替わる）
    int32_t phase_ = 1;                 // 現在のフェーズ（1から）
    int32_t exp_ = 0;                   // 現在のフェーズで溜めた経験値

    // 通知先（所有しない）
    std::function<void()> bossStartCallback_;
    std::function<void()> ruleChangedCallback_;

#ifdef USE_IMGUI
    // フェーズ表の編集画面
    std::unique_ptr<PhaseTableEditor> editor_;
#endif
};
