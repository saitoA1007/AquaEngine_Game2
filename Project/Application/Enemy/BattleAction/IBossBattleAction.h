#pragma once
#include <optional>
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Transform.h"

namespace GameEngine {
    // 前方宣言
    class DebugParameter;
}
class BossRangedAttackManager;
class BossAnimator;

// ボスの戦い中の状態
enum class BossBattleState {
    kRushAttack,    // 突進攻撃
    kWindAttack,    // 風の発射
    kIceFallAttack, // 氷柱を落とす攻撃

    kWait,          // その場で留まる。攻撃と攻撃の小休憩
    kRotateMove,    // 回転して回る動き
    kCrossMove,     // 横断する動き

    kResetMove,     // 位置をリセットする時に使用

    kInMove,        // 最初の時に取る行動

    kMaxCount
};

// 戦いで使用する共通データ
struct BossBattleStateCommonData {
    Transform transform = { {2,2,2},{0,0,0},{0,0,0} };

    // プレイヤーの位置
    const Vector3* playerPos;

    // ステージの半径
    float stageRadius = 20.0f;

    // 状態
    BossBattleState state = BossBattleState::kWait;
    // 状態遷移のリクエスト
    std::optional<BossBattleState> requestState = std::nullopt;

    // アニメーション
    BossAnimator* animator = nullptr;

    // 遠距離攻撃管理
    BossRangedAttackManager* rangedAttackManager = nullptr;
};

/// <summary>
/// ボスのバトル中の行動の基底クラス
/// </summary>
class IBossBattleAction {
public:
    IBossBattleAction(BossBattleStateCommonData& commonData) : commonData_(commonData) {}
    virtual ~IBossBattleAction() = default;

    /// <summary>
    /// 攻撃の初期化
    /// </summary>
    virtual void Initialize() = 0;

    /// <summary>
    /// 攻撃の更新処理
    /// </summary>
    virtual void Update() = 0;

    /// <summary>
    /// 攻撃の終了処理
    /// </summary>
    virtual void Finalize() = 0;

    /// <summary>
    /// 値を登録する
    /// </summary>
    virtual void RegisterParameter([[maybe_unused]] GameEngine::DebugParameter* param) {};

    /// <summary>
    /// 攻撃が完了したかどうか
    /// </summary>
    bool IsFinished() const { return isFinished_; }

protected:
    // 共通データ
    BossBattleStateCommonData& commonData_;

    bool isFinished_ = false;
};