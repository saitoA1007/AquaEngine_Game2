#pragma once
#include "IBossBattleAction.h"

// 突進攻撃
class BossRushAttackAction : public IBossBattleAction {
public:
    // 移動状態
    enum class State {
        kMove,
        kRush,

        kMaxCount
    };

public:
    BossRushAttackAction(BossBattleStateCommonData& commonData);
    ~BossRushAttackAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

    void RegisterParameter(GameEngine::DebugParameter* param) override;

private:
    // 回転移動時間
    float rotateMoveMaxTime_ = 2.0f;

    // 突進する時間
    float rushMaxTime_ = 2.0f;

    // 突進の移動範囲の割合
    float rushDistanceRatio_ = 1.5f;

private:

    State state_ = State::kMove;

    // 回転移動の角度
    float startAngle_ = 0.0f;
    float endAngle_ = 0.0f;
    float angle_ = 0.0f;

    // 突進の位置
    Vector3 startRushPos_;
    Vector3 endRushPos_;

    float defaultPosY_ = 5.0f;

    float timer_ = 0.0f;

private:

    void RotateMove();

    void RushAttack();

};

// 待機
class BossWaitAction : public IBossBattleAction {
public:
    BossWaitAction(BossBattleStateCommonData& commonData);
    ~BossWaitAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

    void RegisterParameter(GameEngine::DebugParameter* param) override;

private:

    float timer_ = 0.0f;
    // 待機時間
    float maxTime_ = 1.0f;
};

// 横断する動き
class BossCrossMoveAction : public IBossBattleAction {
public:

    // 横断の状態
    enum class State {
        kIn,
        kMain,
        kEnd,

        kMaxCount
    };

public:
    BossCrossMoveAction(BossBattleStateCommonData& commonData);
    ~BossCrossMoveAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

    void RegisterParameter(GameEngine::DebugParameter* param) override;

private:

    // はみ出る割合
    float crossEndRatio_ = 1.0f;

    // 最大時間
    float maxTime_ = 4.0f;

    // 移動する時のY軸の基本位置
    float defaultPosY_ = 5.0f;

    // 最大の振れ幅
    float maxMoveHeight_ = 2.0f;

    // 上下移動する回数
    float upDownCount_ = 3.0f;

private:
    State state_ = State::kIn;

    Vector3 startPos_;
    Vector3 endPos_;

    Vector3 startCurrentRotDir_;
    Vector3 endRotDir_;
    Vector3 finalRotDir_;

    float timer_ = 0.0f;
};

// ステージに沿って移動
class RotateMoveAction : public IBossBattleAction {
public:
    RotateMoveAction(BossBattleStateCommonData& commonData);
    ~RotateMoveAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

    void RegisterParameter(GameEngine::DebugParameter* param) override;

private:
    // 移動する時のY軸の基本位置
    float defaultPosY_ = 5.0f;

    // ステージ半径の割合
    float offsetStageRadius_ = 0.0f;

    // 最大の時間
    float maxTime_ = 1.0f;

    // 最大の振れ幅
    float maxMoveHeight_ = 2.0f;

private:
    // 回転移動の角度
    float startAngle_ = 0.0f;
    float endAngle_ = 0.0f;
    float angle_ = 0.0f;

    // 上下する回数
    float cycleCount_ = 0.0f;

    Vector3 startCurrentRotDir_;
    Vector3 endRotDir_;
    Vector3 finalRotDir_;

    float timer_ = 0.0f;
};

// 氷柱攻撃
class IceFallAttackAction : public IBossBattleAction {
public:
    IceFallAttackAction(BossBattleStateCommonData& commonData);
    ~IceFallAttackAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

    void RegisterParameter(GameEngine::DebugParameter* param) override;

private:
    // ステージの使用する範囲割合
    float rangeRadiusRatio_ = 0.8f;

    // 氷柱同士の最小距離
    float minDistance_ = 5.0f;

    // 発生させる数
    int32_t iceFallNum_ = 3;

    // ステージに存在させる最大の氷柱の数
    int32_t iceFallMaxNum_ = 5;

    // 試行回数
    int32_t maxIter_ = 100;

    float maxTime_ = 1.0f;

private:
    float timer_ = 0.0f;

};

// 風攻撃
class WindAttackAction : public IBossBattleAction {
public:

    enum class State {
        kIn,
        kMain,
        kOut,
    };

public:
    WindAttackAction(BossBattleStateCommonData& commonData);
    ~WindAttackAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

    void RegisterParameter(GameEngine::DebugParameter* param) override;

private:

    float inMaxTime_ = 1.0f;
    float mainMaxTime_ = 3.0f;
    float outMaxTime_ = 1.0f;

    float windDirY_ = -0.1f;

private:

    float timer_ = 0.0f;

    Vector3 startCurrentRotDir_;
    
    Vector3 startRotDir_;
    Vector3 endRotDir_;

    State state_ = State::kIn;
};

// リセットアクション
class ResetAction : public IBossBattleAction {
public:

    enum class State {
        kIn,
        kMain,
        kOut,

        kMaxCount
    };

public:
    ResetAction(BossBattleStateCommonData& commonData);
    ~ResetAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

    void RegisterParameter(GameEngine::DebugParameter* param) override;

private:

    // 移動速度。移動時間を求めるのに使用
    float moveSpeed_ = 20.0f;

    // 回転速度。回転する時間を求めるのに使用
    float rotateSpeed_ = 6.0f;

    // 高さ
    float defaultPosY_ = 5.0f;

private:
    Vector3 inStartRotDir_;
    Vector3 inEndRotDir_;

    Vector3 outStartRotDir_;
    Vector3 outEndRotDir_;

    Vector3 startPos_;
    Vector3 endPos_;

    float timer_ = 0.0f;

    float moveMaxTime_ = 0.0f;
    float inRotateMaxTime_ = 1.0f;
    float outRotateMaxTime_ = 1.0f;

    State state_ = State::kIn;
};

/// ヘルパー関数
namespace {
    // 角度からXZ平面上の位置を求める
    Vector3 GetXZFromAngle(float angle, float radius, float posY);

    // ベクトルをXZ平面上で回転させる
    Vector3 RotateVectorXZ(Vector3 dir,float angle);
}
